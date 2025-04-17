#!/usr/bin/env python3
import os
import sys
import time
import datetime
import logging
import instaloader
import csv
import random
from flask import Flask, jsonify
from typing import Dict, Any, Tuple, List, Optional
from dataclasses import dataclass
from threading import Lock

# Set up path to import local modules
current_dir = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(current_dir)

# Import the functions and database module
from get_followers import get_instagram_metrics, is_session_valid
from db.instagram_db import InstagramDatabase

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Constants
DEFAULT_USERNAME = "mein.kreis.pinneberg"
CACHE_FRESHNESS_THRESHOLD = 15 * 60  # 15 minutes
SESSION_DIR = os.path.join(parent_dir, 'data', 'sessions')
CREDENTIALS_FILE = os.path.join(parent_dir, 'data', 'instagram_credentials.csv')
DB_PATH = os.path.join(parent_dir, 'data', 'instagram_metrics.db')

# Initialize Flask app
app = Flask(__name__)

# Global instances
_loader = None
_loader_session_username = None
_credential_manager = None

@dataclass
class InstagramCredential:
    email: str
    username: str
    password: str

class CredentialManager:
    ROTATION_INTERVAL = 30 * 60  # 30 minutes in seconds

    def __init__(self, credentials_file: str):
        self.credentials_file = credentials_file
        self.credentials: List[InstagramCredential] = []
        self.current_credential: Optional[InstagramCredential] = None
        self.last_rotation_time = 0
        self.lock = Lock()
        self._load_credentials()
        self._rotate_credential(force=True)

    def _load_credentials(self) -> None:
        """Load credentials from CSV file"""
        if not os.path.exists(self.credentials_file):
            raise FileNotFoundError(f"Credentials file not found: {self.credentials_file}")
        
        self.credentials = []
        with open(self.credentials_file, 'r') as f:
            reader = csv.reader(f)
            next(reader)  # Skip header row
            for row in reader:
                if len(row) >= 2:
                    email = row[0].strip()
                    creds = row[1].strip().split(':')
                    if len(creds) == 2:
                        username, password = creds
                        self.credentials.append(InstagramCredential(email, username.strip(), password.strip()))

        if not self.credentials:
            raise ValueError("No valid credentials found in the credentials file")
        
        logger.info(f"Loaded {len(self.credentials)} credentials from file")

    def _rotate_credential(self, force: bool = False) -> bool:
        """Rotate to a different credential based on time interval or force parameter"""
        with self.lock:
            current_time = time.time()
            if not force and current_time - self.last_rotation_time < self.ROTATION_INTERVAL:
                return False

            # Select a random credential different from the current one
            available_credentials = [c for c in self.credentials if c != self.current_credential]
            if not available_credentials:
                available_credentials = self.credentials

            self.current_credential = random.choice(available_credentials)
            self.last_rotation_time = current_time
            logger.info(f"Rotated to new credential: {self.current_credential.email}")
            return True

    def get_current_credentials(self) -> Tuple[str, str]:
        """Get the current username and password"""
        if self._rotate_credential():
            logger.info("Rotating credentials due to time interval")
        return self.current_credential.username, self.current_credential.password

def init_credential_manager() -> CredentialManager:
    """Initialize or return the existing credential manager"""
    global _credential_manager
    if _credential_manager is None:
        _credential_manager = CredentialManager(CREDENTIALS_FILE)
    return _credential_manager

def get_loader(force_new: bool = False) -> Optional[instaloader.Instaloader]:
    """
    Get a singleton Instaloader instance or create a new one if needed.
    Uses rotating credentials from the credentials file.
    """
    global _loader, _loader_session_username
    
    try:
        credential_manager = init_credential_manager()
        username, password = credential_manager.get_current_credentials()
        
        # Create a new loader if it doesn't exist, if forced, or if the username has changed
        if _loader is None or force_new or _loader_session_username != username:
            logger.info("Creating a new Instaloader instance")
            _loader = instaloader.Instaloader()
            _loader_session_username = username
            
            # Ensure session directory exists
            os.makedirs(SESSION_DIR, exist_ok=True)
            
            session_filename = f"{username.lower().replace('.', '_')}.session"
            session_path = os.path.join(SESSION_DIR, session_filename)
            
            # Try to load existing session or login if needed
            if is_session_valid(username):
                try:
                    logger.info(f"Attempting to use existing session for {username}...")
                    _loader.load_session_from_file(username, session_path)
                    logger.info("Session loaded successfully")
                    return _loader
                except Exception as e:
                    logger.warning(f"Error loading session, will try to login: {str(e)}")
            
            # If no valid session exists or loading failed, login and save the session
            try:
                logger.info(f"Logging in with account {username}...")
                _loader.login(username, password)
                # Save the session for future use
                _loader.save_session_to_file(session_path)
                logger.info(f"Session saved to {session_path}")
                return _loader
            except Exception as e:
                logger.error(f"Login failed: {str(e)}")
                return None
        
        return _loader
    except Exception as e:
        logger.error(f"Error in get_loader: {str(e)}")
        return None

def get_latest_data(username: str = DEFAULT_USERNAME) -> Dict[str, Any]:
    """
    Get the latest Instagram metrics data, refreshing if older than threshold.
    """
    db = InstagramDatabase(DB_PATH)
    
    try:
        # Get the latest record from the database
        latest_metrics = db.get_latest_metrics(username)
        
        needs_refresh = True
        if latest_metrics:
            followers_count, posts_count, recent_posts_count, collection_date = latest_metrics
            
            # Parse the collection date
            try:
                last_update_time = datetime.datetime.strptime(collection_date, '%Y-%m-%d %H:%M:%S')
                now = datetime.datetime.now()
                time_diff = (now - last_update_time).total_seconds()
                
                # Check if data is fresh enough
                if time_diff < CACHE_FRESHNESS_THRESHOLD:
                    needs_refresh = False
                    logger.info(f"Using cached data from {collection_date} (age: {time_diff:.2f} seconds)")
                else:
                    logger.info(f"Data is stale ({time_diff:.2f} seconds old). Refreshing...")
            except Exception as e:
                logger.error(f"Error parsing collection date: {str(e)}")
        else:
            logger.info("No existing data found. Fetching for the first time...")
        
        # Refresh the data if needed
        if needs_refresh:
            logger.info(f"Refreshing Instagram metrics for {username}...")
            
            # Track retries and set a maximum to prevent infinite loops
            retry_count = 0
            max_retries = min(3, len(_credential_manager.credentials) if _credential_manager else 1)
            
            while retry_count < max_retries:
                # Get or create Instaloader instance
                loader = get_loader(force_new=(retry_count > 0))
                
                if not loader:
                    logger.error("Failed to create or authenticate Instaloader instance")
                    # Return existing data if available, otherwise error
                    if latest_metrics:
                        needs_refresh = False
                        break
                    else:
                        return {
                            "error": True,
                            "message": "Authentication failed, could not retrieve metrics"
                        }
                
                try:
                    metrics, record_id = get_instagram_metrics(
                        username=username, 
                        store_in_db=True, 
                        db_path=DB_PATH,
                        login_username=loader.context.username,
                        loader=loader
                    )
                    
                    if metrics:
                        followers_count = metrics.get('followers')
                        posts_count = metrics.get('posts')
                        recent_posts_count = None  # Not available yet
                        collection_date = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                        logger.info(f"Successfully refreshed metrics with record ID: {record_id}")
                        break  # Success! Exit the retry loop
                    else:
                        logger.warning(f"Failed to get metrics for {username} with account {loader.context.username}")
                        retry_count += 1
                        if retry_count < max_retries:
                            logger.info(f"Rotating to next account and retrying ({retry_count}/{max_retries})")
                        else:
                            logger.error(f"All {max_retries} attempts failed to retrieve metrics")
                            if not latest_metrics:
                                return {
                                    "error": True,
                                    "message": f"Could not retrieve metrics for {username} after {max_retries} attempts"
                                }
                except Exception as e:
                    logger.warning(f"Error refreshing metrics with account {loader.context.username}: {str(e)}")
                    retry_count += 1
                    if retry_count < max_retries:
                        logger.info(f"Rotating to next account and retrying ({retry_count}/{max_retries})")
                    else:
                        logger.error(f"All {max_retries} attempts failed: {str(e)}")
                        if not latest_metrics:
                            return {
                                "error": True,
                                "message": f"Error retrieving metrics after {max_retries} attempts: {str(e)}"
                            }
                        break
        
        # Prepare and return the response
        result = {
            "error": False,
            "username": username,
            "followers_count": followers_count,
            "posts_count": posts_count,
            "recent_posts_count": recent_posts_count,
            "last_updated": collection_date
        }
        
        return result
    finally:
        # Always close the database connection
        db.close()

@app.route('/api/instagram/metrics', methods=['GET'])
def api_get_metrics():
    """API endpoint to get the latest Instagram metrics."""
    data = get_latest_data()
    return jsonify(data)

@app.route('/api/instagram/metrics/<username>', methods=['GET'])
def api_get_metrics_for_user(username):
    """API endpoint to get metrics for a specific username."""
    data = get_latest_data(username)
    return jsonify(data)

@app.route('/health', methods=['GET'])
def health_check():
    """Simple health check endpoint."""
    return jsonify({
        "status": "ok", 
        "timestamp": datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    })

if __name__ == '__main__':
    # Configuration
    port = int(os.environ.get('PORT', 5000))
    host = os.environ.get('HOST', '127.0.0.1')
    
    logger.info(f"Starting Instagram metrics API server on {host}:{port}")
    app.run(host=host, port=port, debug=False)