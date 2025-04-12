import instaloader
import logging
import time
from typing import Optional

def get_follower_count(username: str, retries: int = 3, retry_delay: int = 5) -> Optional[int]:
    """
    Retrieves the follower count for a given Instagram username.
    
    Args:
        username: The Instagram username to check
        retries: Number of retry attempts if connection fails
        retry_delay: Delay between retries in seconds
        
    Returns:
        int: Number of followers if successful
        None: If retrieval fails after all retries
    """
    loader = instaloader.Instaloader()
    
    for attempt in range(retries):
        try:
            profile = instaloader.Profile.from_username(loader.context, username)
            return profile.followers
        except instaloader.exceptions.ConnectionException:
            logging.warning(f"Connection error on attempt {attempt+1}/{retries}. Retrying in {retry_delay} seconds...")
            time.sleep(retry_delay)
        except instaloader.exceptions.ProfileNotExistsException:
            logging.error(f"Profile '{username}' does not exist.")
            return None
        except Exception as e:
            logging.error(f"Error retrieving follower count: {str(e)}")
            return None
    
    logging.error(f"Failed to retrieve follower count after {retries} attempts.")
    return None

# Example usage
if __name__ == "__main__":
    username = "mein.kreis.pinneberg"
    followers = get_follower_count(username)
    
    if followers is not None:
        print(f"Followers for {username}: {followers}")
    else:
        print(f"Could not retrieve follower count for {username}")

