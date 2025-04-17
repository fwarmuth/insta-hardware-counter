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
        try:
            with open(self.credentials_file, 'r') as f:
                reader = csv.reader(f)
                next(reader)  # Skip header row
                for row in reader:
                    if len(row) >= 2:
                        email = row[0].strip()
                        creds = row[1].strip().split(':')
                        if len(creds) == 2:
                            username, password = creds
                            self.credentials.append(InstagramCredential(
                                email, 
                                username.strip(), 
                                password.strip()
                            ))
            
            if not self.credentials:
                raise ValueError("No valid credentials found in the credentials file")
            
            logger.info(f"Loaded {len(self.credentials)} credentials from file")
        except Exception as e:
            logger.error(f"Error loading credentials: {str(e)}")
            raise

    def _rotate_credential(self, force: bool = False) -> bool:
        """Rotate to a different credential based on time interval or force parameter"""
        with self.lock:
            current_time = time.time()
            
            # Check if rotation is needed
            if not force and current_time - self.last_rotation_time < self.ROTATION_INTERVAL:
                return False

            # If we have more than one credential, try to select a different one
            if len(self.credentials) > 1:
                available_credentials = [c for c in self.credentials if c != self.current_credential]
            else:
                available_credentials = self.credentials

            self.current_credential = random.choice(available_credentials)
            self.last_rotation_time = current_time
            logger.info(f"Rotated to credential: {self.current_credential.email}")
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
    
    Args:
        force_new: If True, forces creation of a new instance
        
    Returns:
        An authenticated Instaloader instance, or None if authentication fails
    """
    global _loader, _loader_session_username
    
    try:
        # Get credentials
        credential_manager = init_credential_manager()
        username, password = credential_manager.get_current_credentials()
        
        # Return existing loader if it's valid
        if not force_new and _loader and _loader_session_username == username:
            return _loader
            
        # Create a new loader instance
        logger.info("Creating a new Instaloader instance")
        _loader = instaloader.Instaloader()
        _loader_session_username = username
        
        # Ensure session directory exists
        os.makedirs(SESSION_DIR, exist_ok=True)
        
        # Set up session path
        session_filename = f"{username.lower().replace('.', '_')}.session"
        session_path = os.path.join(SESSION_DIR, session_filename)
        
        # Try to use existing session if valid
        if is_session_valid(username):
            try:
                logger.info(f"Using existing session for {username}")
                _loader.load_session_from_file(username, session_path)
                logger.info("Session loaded successfully")
                return _loader
            except Exception as e:
                logger.warning(f"Could not load session: {str(e)}")
                # Continue to login attempt
        
        # Login and save session
        try:
            logger.info(f"Logging in with account {username}")
            _loader.login(username, password)
            _loader.save_session_to_file(session_path)
            logger.info(f"New session saved to {session_path}")
            return _loader
        except Exception as e:
            logger.error(f"Login failed: {str(e)}")
            _loader = None
            _loader_session_username = None
            return None
            
    except Exception as e:
        logger.error(f"Error in get_loader: {str(e)}")
        _loader = None
        _loader_session_username = None
        return None

def refresh_metrics_data(username: str) -> Tuple[Dict[str, Any], bool]:
    """
    Attempt to refresh Instagram metrics for a username.
    Returns a tuple of (result_dict, success_flag)
    """
    logger.info(f"Refreshing Instagram metrics for {username}...")
    
    # Initialize credential manager if needed
    credential_manager = init_credential_manager()
    
    # Track retries and set a maximum to prevent infinite loops
    retry_count = 0
    max_retries = min(3, len(credential_manager.credentials))
    
    while retry_count < max_retries:
        # Get or create Instaloader instance, forcing new instance after a failed attempt
        loader = get_loader(force_new=(retry_count > 0))
        
        if not loader:
            logger.error("Failed to create or authenticate Instaloader instance")
            return {
                "error": True,
                "message": "Authentication failed, could not retrieve metrics"
            }, False
        
        try:
            metrics, record_id = get_instagram_metrics(
                username=username, 
                store_in_db=True, 
                db_path=DB_PATH,
                login_username=loader.context.username,
                loader=loader
            )
            
            if not metrics:
                logger.warning(f"Failed to get metrics for {username} with account {loader.context.username}")
                retry_count += 1
                if retry_count < max_retries:
                    logger.info(f"Rotating to next account and retrying ({retry_count}/{max_retries})")
                continue
                
            # Success! Return data
            collection_date = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            return {
                "error": False,
                "username": username,
                "followers_count": metrics.get('followers'),
                "posts_count": metrics.get('posts'),
                "recent_posts_count": None,  # Not available yet
                "last_updated": collection_date
            }, True
                
        except Exception as e:
            logger.warning(f"Error refreshing metrics with account {loader.context.username}: {str(e)}")
            retry_count += 1
            if retry_count < max_retries:
                logger.info(f"Rotating to next account and retrying ({retry_count}/{max_retries})")

    # If we get here, all retries failed
    logger.error(f"All {max_retries} attempts failed to retrieve metrics")
    return {
        "error": True,
        "message": f"Could not retrieve metrics for {username} after {max_retries} attempts"
    }, False

def get_latest_data(username: str = DEFAULT_USERNAME) -> Dict[str, Any]:
    """
    Get the latest Instagram metrics data, refreshing if older than threshold.
    Uses cached data if available and fresh enough, otherwise tries to refresh.
    """
    # Create a database context
    db = InstagramDatabase(DB_PATH)
    
    try:
        # Get the latest record from the database
        latest_metrics = db.get_latest_metrics(username)
        
        # Default to needing a refresh
        needs_refresh = True
        
        # Check if we have cached data and if it's fresh enough
        if latest_metrics:
            followers_count, posts_count, recent_posts_count, collection_date = latest_metrics
            
            try:
                last_update_time = datetime.datetime.strptime(collection_date, '%Y-%m-%d %H:%M:%S')
                now = datetime.datetime.now()
                time_diff = (now - last_update_time).total_seconds()
                
                if time_diff < CACHE_FRESHNESS_THRESHOLD:
                    needs_refresh = False
                    logger.info(f"Using cached data from {collection_date} (age: {time_diff:.2f} seconds)")
                else:
                    logger.info(f"Data is stale ({time_diff:.2f} seconds old). Refreshing...")
            except Exception as e:
                logger.error(f"Error parsing collection date: {str(e)}")
        else:
            logger.info("No existing data found. Fetching for the first time...")
        
        # Refresh data if needed
        if needs_refresh:
            refreshed_data, success = refresh_metrics_data(username)
            
            # If refresh succeeded, return the new data
            if success:
                return refreshed_data
                
            # If refresh failed but we have cached data, use that
            if latest_metrics:
                logger.warning("Refresh failed. Falling back to cached data.")
                return {
                    "error": False,
                    "username": username,
                    "followers_count": followers_count,
                    "posts_count": posts_count,
                    "recent_posts_count": recent_posts_count,
                    "last_updated": collection_date,
                    "note": "Data refresh failed, showing cached data"
                }
            
            # No cached data and refresh failed
            return refreshed_data
        
        # Using cached data
        return {
            "error": False,
            "username": username,
            "followers_count": followers_count,
            "posts_count": posts_count,
            "recent_posts_count": recent_posts_count,
            "last_updated": collection_date
        }
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