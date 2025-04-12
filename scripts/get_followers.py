#!/usr/bin/env python3
import re
import sys
import time
import os
import datetime
from pathlib import Path

# Add the script directory to Python path to import the database module
script_dir = Path(__file__).resolve().parent
sys.path.append(str(script_dir))

# Import the database module
try:
    from db.instagram_db import InstagramDatabase
except ImportError:
    print("Error: Could not import database module.")
    print("Make sure the db/instagram_db.py file exists.")

try:
    import instaloader
except ImportError:
    print("Error: instaloader library not found.")
    print("Please install it using: pip install instaloader")
    sys.exit(1)

def parse_number(number_str):
    """
    Convert a number string with optional shorthand (k, m) into an integer.
    Example: '1,234' -> 1234, '1.2m' -> 1200000, '3.4k' -> 3400.
    """
    number_str = number_str.lower().replace(',', '').strip()
    if 'm' in number_str:
        try:
            return int(float(number_str.replace('m', '')) * 1_000_000)
        except ValueError:
            pass
    elif 'k' in number_str:
        try:
            return int(float(number_str.replace('k', '')) * 1_000)
        except ValueError:
            pass
    else:
        try:
            return int(number_str)
        except ValueError:
            pass
    raise ValueError(f"Could not parse number from string: {number_str}")

def get_instagram_followers(username):
    """
    Get the exact follower count, total post count, and recent posts
    for an Instagram profile using instaloader.
    
    Args:
        username (str): Instagram username without the @ symbol
        
    Returns:
        tuple: (followers_count, posts_count, recent_posts_count)
            - followers_count: The exact number of followers
            - posts_count: The total number of posts
            - recent_posts_count: Number of posts in the last 7 days
    """
    # Create an instance of Instaloader
    loader = instaloader.Instaloader()
    
    try:
        # Get profile information
        profile = instaloader.Profile.from_username(loader.context, username)
        
        # Get total posts count
        posts_count = profile.mediacount
        
        # Count posts from the last 7 days
        recent_posts_count = 0
        current_time = time.time()
        seven_days_ago = current_time - (7 * 24 * 60 * 60)  # 7 days in seconds
        
        # Iterate through the most recent posts and count those from the last 7 days
        for post in profile.get_posts():
            if post.date_utc.timestamp() > seven_days_ago:
                recent_posts_count += 1
            else:
                # Stop once we reach posts older than 7 days
                break
        
        # Return the follower count, total posts count, and recent posts count
        return profile.followers, posts_count, recent_posts_count
        
    except instaloader.exceptions.ProfileNotExistsException:
        raise Exception(f"Profile '{username}' does not exist")
    except instaloader.exceptions.ConnectionException:
        raise Exception("Connection error. Instagram may be rate-limiting your requests")
    except Exception as e:
        raise Exception(f"Error accessing Instagram profile: {str(e)}")

def get_instagram_followers_fallback(url):
    """
    Fallback method using requests and BeautifulSoup.
    This method extracts follower count and post count from the Instagram profile.
    
    Returns:
        tuple: (followers_count, posts_count, None)
            - followers_count: The approximate number of followers
            - posts_count: The approximate number of posts
            - None for recent posts (not available in fallback method)
    """
    import requests
    from bs4 import BeautifulSoup
    
    # Use a browser-like User-Agent to reduce the risk of blocking.
    headers = {
        "User-Agent": (
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
            "AppleWebKit/537.36 (KHTML, like Gecko) "
            "Chrome/58.0.3029.110 Safari/537.3"
        )
    }
    
    response = requests.get(url, headers=headers)
    if response.status_code != 200:
        raise Exception(f"Failed to load page, status code: {response.status_code}")
    
    # Use BeautifulSoup to parse the HTML.
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # Find the meta tag with property "og:description".
    meta_tag = soup.find('meta', property='og:description')
    if not meta_tag:
        raise Exception("Could not find the meta tag with property 'og:description'. The page structure may have changed.")
    
    content = meta_tag.get('content', '')
    if not content:
        raise Exception("The meta tag did not contain any content.")
    
    # The content is expected to be in the format:
    # "1,234 Followers, 567 Following, 89 Posts - See more"
    
    # Extract follower count
    follower_match = re.search(r'([\d.,]+[kKmM]?)\s+Followers', content)
    if not follower_match:
        raise Exception("Could not parse follower count from the meta tag content.")
    
    follower_count_str = follower_match.group(1)
    follower_count = parse_number(follower_count_str)
    
    # Extract post count
    post_match = re.search(r'([\d.,]+[kKmM]?)\s+Posts', content)
    if not post_match:
        # If posts can't be extracted, return None for posts
        return follower_count, None, None
    
    post_count_str = post_match.group(1)
    post_count = parse_number(post_count_str)
    
    # Fallback method can't determine recent posts, so return None for that value
    return follower_count, post_count, None

if __name__ == '__main__':
    url = 'https://www.instagram.com/mein.kreis.pinneberg/'
    username = 'mein.kreis.pinneberg'  # Extract username from the URL
    
    # Initialize database connection
    db = InstagramDatabase()
    current_timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    try:
        # Try the more accurate method first
        print("Attempting to get exact follower count using instaloader...")
        followers, posts, recent_posts = get_instagram_followers(username)
        print("Exact follower count:", followers)
        print("Total posts count:", posts)
        print("Recent posts count (last 7 days):", recent_posts)
        
        # Store data in database with high resolution timestamp
        record_id = db.store_metrics(username, followers, posts, recent_posts, current_timestamp)
        print(f"Data stored in database with ID: {record_id}")
        
    except Exception as insta_error:
        print(f"Error with instaloader method: {insta_error}")
        print("Falling back to web scraping method...")
        try:
            # Fall back to the original method if the first one fails
            followers, posts, recent_posts = get_instagram_followers_fallback(url)
            print("Approximate follower count:", followers)
            print("Approximate posts count:", posts)
            print("Note: These counts may be rounded. For exact counts, try resolving the instaloader issues.")
            
            # Store data in database with high resolution timestamp
            record_id = db.store_metrics(username, followers, posts, recent_posts, current_timestamp)
            print(f"Data stored in database with ID: {record_id}")
            
        except Exception as error:
            print("Error with fallback method:", error)
    
    # Display historical data if available
    try:
        print("\nHistorical Data (Last 5 records):")
        historical_data = db.get_historical_data(username, 5)
        if historical_data:
            print("Timestamp\t\t\tFollowers\tPosts\tRecent Posts")
            print("-" * 70)
            for record in historical_data:
                followers, posts, recent, timestamp = record
                recent_str = str(recent) if recent is not None else "N/A"
                print(f"{timestamp}\t{followers}\t\t{posts}\t{recent_str}")
        else:
            print("No historical data available yet.")
    except Exception as db_error:
        print(f"Error retrieving historical data: {db_error}")
    
    # Close the database connection
    db.close()

