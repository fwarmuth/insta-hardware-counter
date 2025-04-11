#!/usr/bin/env python3
import re
import sys
import time
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
    Get the exact follower count for an Instagram profile using instaloader.
    
    Args:
        username (str): Instagram username without the @ symbol
        
    Returns:
        int: The exact number of followers
    """
    # Create an instance of Instaloader
    loader = instaloader.Instaloader()
    
    try:
        # Get profile information
        profile = instaloader.Profile.from_username(loader.context, username)
        
        # Return the exact follower count
        return profile.followers
        
    except instaloader.exceptions.ProfileNotExistsException:
        raise Exception(f"Profile '{username}' does not exist")
    except instaloader.exceptions.ConnectionException:
        raise Exception("Connection error. Instagram may be rate-limiting your requests")
    except Exception as e:
        raise Exception(f"Error accessing Instagram profile: {str(e)}")

def get_instagram_followers_fallback(url):
    """
    Fallback method using requests and BeautifulSoup.
    This is the original method and will be used if instaloader fails.
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
    # We want to extract the follower count.
    parts = content.split(',')
    if not parts:
        raise Exception("Unexpected format of the meta tag content.")
    
    # Extract the first part which should include the follower information.
    followers_part = parts[0]
    # Use regex to find the number (optionally with commas or shorthand like k/m)
    match = re.search(r'([\d.,]+[kKmM]?)', followers_part)
    if not match:
        raise Exception("Could not parse follower count from the meta tag content.")
    
    count_str = match.group(1)
    follower_count = parse_number(count_str)
    
    return follower_count

if __name__ == '__main__':
    url = 'https://www.instagram.com/mein.kreis.pinneberg/'
    username = 'mein.kreis.pinneberg'  # Extract username from the URL
    
    try:
        # Try the more accurate method first
        print("Attempting to get exact follower count using instaloader...")
        count = get_instagram_followers(username)
        print("Exact follower count:", count)
    except Exception as insta_error:
        print(f"Error with instaloader method: {insta_error}")
        print("Falling back to web scraping method...")
        try:
            # Fall back to the original method if the first one fails
            count = get_instagram_followers_fallback(url)
            print("Approximate follower count:", count)
            print("Note: This count may be rounded. For exact counts, try resolving the instaloader issues.")
        except Exception as error:
            print("Error with fallback method:", error)

