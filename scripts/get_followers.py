import instaloader
import logging
import time
import os
import sys
import datetime
from typing import Optional, Tuple, Dict

# Add scripts directory to path to import the database module
current_dir = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(current_dir)

# Import the database module
from db.instagram_db import InstagramDatabase

def get_instagram_metrics(
    username: str, 
    store_in_db: bool = True,
    db_path: str = None,
    retries: int = 3, 
    retry_delay: int = 5
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
    
    # Get metrics from Instagram
    loader = instaloader.Instaloader()
    
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
    username = "mein.kreis.pinneberg"
    metrics, record_id = get_instagram_metrics(username)
    
    if metrics is not None:
        print(f"Instagram metrics for {username}:")
        print(f"  - Followers: {metrics['followers']}")
        print(f"  - Posts: {metrics['posts']}")
        
        if record_id is not None:
            print(f"Data stored in database with record ID: {record_id}")
    else:
        print(f"Could not retrieve metrics for {username}")

