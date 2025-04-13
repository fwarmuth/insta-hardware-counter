import instaloader
import logging
import time
import os
import sys
import datetime
import argparse
from typing import Optional, Tuple, Dict
from pathlib import Path

# Add scripts directory to path to import the database module
current_dir = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(current_dir)

# Import the database module
from db.instagram_db import InstagramDatabase

# Define the session directory and file name format
SESSION_DIR = os.path.join(parent_dir, 'data', 'sessions')

def is_session_valid(username: str) -> bool:
    """
    Check if a session file exists and is less than a day old.
    
    Args:
        username: Instagram login username
        
    Returns:
        bool: True if a valid session exists, False otherwise
    """
    if not username:
        return False
    
    # Create a safe filename from the username
    session_filename = f"{username.lower().replace('.', '_')}.session"
    session_path = os.path.join(SESSION_DIR, session_filename)
    
    # Check if session file exists
    if not os.path.exists(session_path):
        return False
    
    # Check if session is less than a day old
    file_mtime = os.path.getmtime(session_path)
    last_modified = datetime.datetime.fromtimestamp(file_mtime)
    now = datetime.datetime.now()
    age_hours = (now - last_modified).total_seconds() / 3600
    
    # Session is valid if less than 24 hours old
    return age_hours < 24

def get_instagram_metrics(
    username: str, 
    store_in_db: bool = True,
    db_path: str = None,
    retries: int = 1, 
    retry_delay: int = 5,
    login_username: str = None,
    login_password: str = None
) -> Tuple[Optional[Dict[str, int]], Optional[int]]:
    """
    Retrieves metrics (follower count and post count) for a given Instagram username 
    and stores them in the database.
    
    Args:
        username: The Instagram username to check
        store_in_db: Whether to store the result in the database
        db_path: Path to the database file (defaults to data/instagram_metrics.db)
        retries: Number of retry attempts if connection fails
        retry_delay: Delay between retries in seconds
        login_username: Instagram username for login
        login_password: Instagram password for login
        
    Returns:
        Tuple[Optional[Dict[str, int]], Optional[int]]: 
            - First value: Dictionary with 'followers' and 'posts' counts if successful, None if retrieval fails
            - Second value: Database record ID if stored, None if not stored or storage fails
    """
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )
    
    # Default database path is relative to the project root
    if db_path is None:
        db_path = os.path.join(parent_dir, 'data', 'instagram_metrics.db')
    
    metrics = None
    db_record_id = None
    
    # Create sessions directory if it doesn't exist
    os.makedirs(SESSION_DIR, exist_ok=True)
    
    # Get metrics from Instagram
    loader = instaloader.Instaloader()
    
    # Set up session filename if we have login credentials
    session_filename = None
    if login_username:
        session_filename = f"{login_username.lower().replace('.', '_')}.session"
        session_path = os.path.join(SESSION_DIR, session_filename)
    
    # Try to load existing session or login if needed
    if login_username and login_password:
        session_loaded = False
        
        # Check if there's a valid session file
        if is_session_valid(login_username):
            try:
                logging.info(f"Attempting to use existing session for {login_username}...")
                loader.load_session_from_file(login_username, session_path)
                session_loaded = True
                logging.info("Session loaded successfully")
            except Exception as e:
                logging.warning(f"Error loading session, will try to login: {str(e)}")
                session_loaded = False
        
        # If no valid session exists or loading failed, login and save the session
        if not session_loaded:
            try:
                logging.info(f"Logging in with account {login_username}...")
                loader.login(login_username, login_password)
                # Save the session for future use
                loader.save_session_to_file(session_path)
                logging.info(f"Session saved to {session_path}")
            except Exception as e:
                logging.error(f"Login failed: {str(e)}")
                return None, None
    else:
        logging.warning("WARNING: No Instagram login credentials provided. This may lead to rate limiting or restricted access to data. Consider setting INSTAGRAM_USERNAME and INSTAGRAM_PASSWORD environment variables.")
    
    for attempt in range(retries):
        try:
            logging.info(f"Retrieving metrics for {username} (attempt {attempt+1}/{retries})...")
            profile = instaloader.Profile.from_username(loader.context, username)
            
            # Get both follower count and post count
            follower_count = profile.followers
            post_count = profile.mediacount
            
            metrics = {
                'followers': follower_count,
                'posts': post_count
            }
            
            logging.info(f"Successfully retrieved metrics for {username}: {metrics}")
            break
        except instaloader.exceptions.ConnectionException:
            logging.warning(f"Connection error on attempt {attempt+1}/{retries}. Retrying in {retry_delay} seconds...")
            time.sleep(retry_delay)
        except instaloader.exceptions.ProfileNotExistsException:
            logging.error(f"Profile '{username}' does not exist.")
            return None, None
        except Exception as e:
            logging.error(f"Error retrieving metrics: {str(e)}")
            return None, None
    
    if metrics is None:
        logging.error(f"Failed to retrieve metrics after {retries} attempts.")
        return None, None
    
    # Store in database if requested
    if store_in_db and metrics is not None:
        try:
            # Create timestamp
            timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            # Store in database
            db = InstagramDatabase(db_path)
            # Store both follower count and post count, leave recent_posts_count as None for now
            db_record_id = db.store_metrics(
                username=username,
                followers_count=metrics['followers'],
                posts_count=metrics['posts'],
                recent_posts_count=None,
                timestamp=timestamp
            )
            db.close()
            
            logging.info(f"Stored metrics in database with record ID: {db_record_id}")
        except Exception as e:
            logging.error(f"Error storing metrics in database: {str(e)}")
    
    return metrics, db_record_id

# For backward compatibility
def get_follower_count(username: str, store_in_db: bool = True, db_path: str = None, retries: int = 3, retry_delay: int = 5):
    """Backward compatibility function that calls get_instagram_metrics but returns only follower count."""
    metrics, record_id = get_instagram_metrics(username, store_in_db, db_path, retries, retry_delay)
    if metrics is None:
        return None, None
    return metrics['followers'], record_id

# Example usage
if __name__ == "__main__":
    # Set up argument parser
    parser = argparse.ArgumentParser(description='Retrieve Instagram metrics.')
    parser.add_argument('username', type=str, nargs='?', 
                        help='Instagram username to retrieve metrics for')
    parser.add_argument('--login_username', type=str, 
                        help='Instagram login username')
    parser.add_argument('--login_password', type=str, 
                        help='Instagram login password')
    args = parser.parse_args()
    
    # Get values from environment variables if not provided in command line
    target_username = args.username or os.environ.get('INSTAGRAM_TARGET_ACCOUNT')
    login_username = args.login_username or os.environ.get('INSTAGRAM_USERNAME')
    login_password = args.login_password or os.environ.get('INSTAGRAM_PASSWORD')
    
    # Check if we have a target username
    if not target_username:
        print("Error: Target Instagram account not specified. Please provide it as an argument or set INSTAGRAM_TARGET_ACCOUNT environment variable.")
        sys.exit(1)
    
    metrics, record_id = get_instagram_metrics(
        target_username, 
        login_username=login_username, 
        login_password=login_password
    )
    
    if metrics is not None:
        print(f"Instagram metrics for {target_username}:")
        print(f"  - Followers: {metrics['followers']}")
        print(f"  - Posts: {metrics['posts']}")
        
        if record_id is not None:
            print(f"Data stored in database with record ID: {record_id}")
    else:
        print(f"Could not retrieve metrics for {target_username}")

