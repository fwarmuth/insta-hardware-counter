import instaloader
import time
import random
import logging
import os
import sys
from datetime import datetime

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

logger.info(f"Instaloader version: {instaloader.__version__}")

# Authentication settings
username = "counter9422"
# Try the session file in the scripts directory first
script_dir = os.path.dirname(os.path.abspath(__file__))
local_session_path = os.path.join(script_dir, "session-counter9422")
default_session_path = f"/home/felix/.config/instaloader/session-counter9422"
profile_to_check = "mein.kreis.pinneberg"

# Custom user agents to mimic real browser
USER_AGENTS = [
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/92.0.4515.159 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Safari/605.1.15"
]

def get_profile_data(session_path, attempt=1, max_attempts=3):
    """Get profile data using instaloader directly"""
    # Create a fresh instance for each attempt
    loader = instaloader.Instaloader(
        download_pictures=False,
        download_videos=False, 
        download_video_thumbnails=False,
        download_geotags=False,
        download_comments=False,
        save_metadata=False,
        compress_json=False,
        user_agent=random.choice(USER_AGENTS)  # Use random user agent
    )
    
    try:
        # Check if session file exists
        if not os.path.exists(session_path):
            logger.error(f"Session file not found at {session_path}")
            return None
            
        # Load session
        logger.info(f"Loading session from {session_path} (attempt {attempt}/{max_attempts})")
        loader.load_session_from_file(username, session_path)
        
        # Add a significant delay to avoid rate limits (increasing with each attempt)
        delay = 5 * attempt + random.uniform(5, 10)
        logger.info(f"Waiting {delay:.2f} seconds before request...")
        time.sleep(delay)
        
        # Get profile 
        logger.info(f"Fetching profile data for {profile_to_check}...")
        profile = instaloader.Profile.from_username(loader.context, profile_to_check)
        
        logger.info(f"Success! Profile {profile.username} has {profile.mediacount} posts and {profile.followers} followers.")
        return profile
    except instaloader.exceptions.LoginRequiredException:
        logger.error("Login required - session might be invalid or expired")
        return None
    except instaloader.exceptions.ConnectionException as e:
        error_msg = str(e)
        logger.error(f"Connection error: {error_msg}")
        
        # Check if we should retry
        if attempt < max_attempts and "Please wait a few minutes before you try again" in error_msg:
            logger.info(f"Rate limited. Will retry after longer delay (attempt {attempt}/{max_attempts})")
            time.sleep(30 * attempt)  # Exponential backoff
            return get_profile_data(session_path, attempt + 1, max_attempts)
        return None
    except Exception as e:
        logger.error(f"Error fetching profile: {str(e)}")
        return None

def try_with_new_session(session_path):
    """Try to create a new session and fetch data"""
    loader = instaloader.Instaloader(user_agent=random.choice(USER_AGENTS))
    try:
        logger.info("Creating new session with interactive login")
        loader.interactive_login(username)
        loader.save_session_to_file(session_path)
        logger.info(f"New session saved to {session_path}")
        
        # Add a significant delay before using the new session
        delay = random.uniform(10, 15)
        logger.info(f"Waiting {delay:.2f} seconds before using new session...")
        time.sleep(delay)
        
        # Try to get profile with new session
        profile = instaloader.Profile.from_username(loader.context, profile_to_check)
        logger.info(f"Success with new session! Profile {profile.username} has {profile.mediacount} posts and {profile.followers} followers.")
        return True
    except Exception as e:
        logger.error(f"Error with new session: {str(e)}")
        return False

# Main execution
logger.info("Starting Instagram profile fetch")

# First try with the local session file in scripts/ directory
profile = get_profile_data(local_session_path)

# If that fails, try with the default session location
if not profile:
    logger.info("Local session failed, trying default session location...")
    profile = get_profile_data(default_session_path)

# If both existing sessions fail, try with a new session
if not profile:
    logger.info("Attempting to create a new session...")
    # Try to save to the local scripts directory first
    success = try_with_new_session(local_session_path)
    
    if not success:
        logger.error("All attempts failed. This could be due to:")
        logger.error("1. Instagram rate limiting - wait at least 30 minutes before trying again")
        logger.error("2. Network issues - check your internet connection and any firewalls/proxies")
        logger.error("3. Account issues - your account might be temporarily restricted")
        sys.exit(1)

