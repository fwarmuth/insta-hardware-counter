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
from typing import Dict, Any, Tuple, List
from dataclasses import dataclass
from threading import Lock

# Add the scripts directory to the path to import our modules
current_dir = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(current_dir)

# Import the functions and database module
from get_followers import get_instagram_metrics, is_session_valid
from db.instagram_db import InstagramDatabase

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

# Initialize Flask app
app = Flask(__name__)

# Default Instagram username (can be configured)
DEFAULT_USERNAME = "mein.kreis.pinneberg"
# Cache freshness threshold (in seconds)
CACHE_FRESHNESS_THRESHOLD = 5*60  # 5 minutes

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
        self.current_credential: InstagramCredential = None
        self.last_rotation_time = 0
        self.lock = Lock()
        self._load_credentials()
        self._rotate_credential(force=True)

    def _load_credentials(self):
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

    def _rotate_credential(self, force: bool = False) -> bool:
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
        with self.lock:
            if self._rotate_credential():
                logger.info("Rotating credentials due to time interval")
            return self.current_credential.username, self.current_credential.password

def init_credential_manager():
    global _credential_manager
    if _credential_manager is None:
        credentials_file = os.path.join(parent_dir, 'data', 'instagram_credentials.csv')
        _credential_manager = CredentialManager(credentials_file)
    return _credential_manager

def get_loader(force_new=False):
    """
    Get a singleton Instaloader instance or create a new one if needed.
    Uses rotating credentials from the credentials file.
    
    Args:
        force_new: Force creation of a new loader instance
        
    Returns:
        Instaloader instance
    """
    global _loader, _loader_session_username, _credential_manager
    
    try:
        credential_manager = init_credential_manager()
        username, password = credential_manager.get_current_credentials()
        
        # Create a new loader if it doesn't exist, if forced, or if the username has changed
        if _loader is None or force_new or _loader_session_username != username:
            logger.info("Creating a new Instaloader instance")
            _loader = instaloader.Instaloader()
            _loader_session_username = username
            
            # Set up session path
            session_dir = os.path.join(parent_dir, 'data', 'sessions')
            os.makedirs(session_dir, exist_ok=True)
            
            session_filename = f"{username.lower().replace('.', '_')}.session"
            session_path = os.path.join(session_dir, session_filename)
            
            # Try to load existing session or login if needed
            session_loaded = False
            
            # Check if there's a valid session file
            if is_session_valid(username):
                try:
                    logger.info(f"Attempting to use existing session for {username}...")
                    _loader.load_session_from_file(username, session_path)
                    session_loaded = True
                    logger.info("Session loaded successfully")
                except Exception as e:
                    logger.warning(f"Error loading session, will try to login: {str(e)}")
                    session_loaded = False
            
            # If no valid session exists or loading failed, login and save the session
            if not session_loaded:
                try:
                    logger.info(f"Logging in with account {username}...")
                    _loader.login(username, password)
                    # Save the session for future use
                    _loader.save_session_to_file(session_path)
                    logger.info(f"Session saved to {session_path}")
                except Exception as e:
                    logger.error(f"Login failed: {str(e)}")
                    return None
        
        return _loader
    except Exception as e:
        logger.error(f"Error in get_loader: {str(e)}")
        return None

def get_latest_data(username: str = DEFAULT_USERNAME) -> Dict[str, Any]:
    """
    Get the latest Instagram metrics data, refreshing if it's older than the threshold.
    
    Args:
        username: The Instagram username to check
        
    Returns:
        Dict containing the metrics data
    """
    db_path = os.path.join(parent_dir, 'data', 'instagram_metrics.db')
    db = InstagramDatabase(db_path)
    
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
            
            # Get or create Instaloader instance
            loader = get_loader()
            
            # Check if we have a valid loader
            if not loader:
                logger.error("Failed to create or authenticate Instaloader instance")
                # If we have no loader but old data exists, use it
                if latest_metrics:
                    needs_refresh = False
                else:
                    # No data at all, return error status
                    return {
                        "error": True,
                        "message": "Authentication failed, could not retrieve metrics"
                    }
            
            if needs_refresh and loader:
                metrics, record_id = get_instagram_metrics(
                    username, 
                    store_in_db=True, 
                    db_path=db_path,
                    login_username=loader.context.username,
                    login_password=None,
                    loader=loader  # Pass the existing loader instance
                )
                
                if metrics:
                    followers_count = metrics.get('followers')
                    posts_count = metrics.get('posts')
                    # We don't have recent posts count yet, so leave it as None
                    recent_posts_count = None
                    collection_date = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    logger.info(f"Successfully refreshed metrics with record ID: {record_id}")
                else:
                    logger.error(f"Failed to refresh metrics for {username}")
                    # If refresh failed but we have old data, use it
                    if not latest_metrics:
                        # No data at all, return error status
                        return {
                            "error": True,
                            "message": f"Could not retrieve metrics for {username}"
                        }
        
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
    """API endpoint to get the latest Instagram metrics for a specific username."""
    data = get_latest_data(username)
    return jsonify(data)

@app.route('/health', methods=['GET'])
def health_check():
    """Simple health check endpoint."""
    return jsonify({"status": "ok", "timestamp": datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')})

# Main entry point
if __name__ == '__main__':
    # Default port is 5000
    port = int(os.environ.get('PORT', 5000))
    # By default, only accessible from localhost. Set host to '0.0.0.0' to make it accessible externally
    host = os.environ.get('HOST', '127.0.0.1')
    
    # Check if Instagram credentials are provided
    instagram_username = os.environ.get('INSTAGRAM_USERNAME')
    instagram_password = os.environ.get('INSTAGRAM_PASSWORD')
    
    # Print environment variables for debugging
    logger.info("Environment Variables:")
    logger.info(f"HOST: {os.environ.get('HOST', 'Not set')}")
    logger.info(f"PORT: {os.environ.get('PORT', 'Not set')}")
    logger.info(f"INSTAGRAM_USERNAME: {instagram_username if instagram_username else 'Not set'}")
    logger.info(f"INSTAGRAM_PASSWORD: {'*****' if instagram_password else 'Not set'}")
    
    # Print all environment variables for debugging
    logger.info("All Environment Variables:")
    for key, value in os.environ.items():
        # Mask sensitive values
        if 'password' in key.lower() or 'secret' in key.lower() or 'key' in key.lower():
            logger.info(f"{key}: *****")
        else:
            logger.info(f"{key}: {value}")
    
    if not instagram_username or not instagram_password:
        logger.warning("WARNING: No Instagram login credentials provided. This may lead to rate limiting or restricted access to data.")
        logger.warning("Consider setting INSTAGRAM_USERNAME and INSTAGRAM_PASSWORD environment variables.")
    
    logger.info(f"Starting Instagram metrics API server on {host}:{port}")
    app.run(host=host, port=port, debug=False)