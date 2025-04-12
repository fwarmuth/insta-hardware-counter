#!/usr/bin/env python3
import os
import sys
import argparse
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
    sys.exit(1)

# Import custom functions for getting Instagram metrics
from get_followers import get_instagram_followers, get_instagram_followers_fallback, get_instagram_followers_api

def load_config():
    """Load API credentials from configuration file"""
    import json
    config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'config.json')
    
    try:
        if os.path.exists(config_path):
            with open(config_path, 'r') as f:
                return json.load(f)
        else:
            return {}
    except Exception as e:
        print(f"Error loading config: {e}")
        return {}

def main():
    parser = argparse.ArgumentParser(description='Fetch Instagram metrics using different methods')
    parser.add_argument('username', type=str, help='Instagram username without @ symbol')
    parser.add_argument('--url', type=str, help='Instagram profile URL (for fallback method)')
    parser.add_argument('--method', type=str, choices=['instaloader', 'api', 'fallback'], 
                        default='instaloader', help='Method to fetch Instagram metrics')
    args = parser.parse_args()
    
    # Initialize database
    db = InstagramDatabase()
    current_timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    if args.method == 'api':
        # Load API credentials from config file
        config = load_config()
        access_token = config.get('access_token')
        business_id = config.get('instagram_business_id')
        
        if not access_token or not business_id:
            print("Error: Missing API credentials in config.json")
            print("Please create a config.json file with 'access_token' and 'instagram_business_id' fields")
            sys.exit(1)
        
        try:
            print(f"Fetching metrics for @{args.username} using Instagram Graph API...")
            followers, posts, recent_posts = get_instagram_followers_api(
                args.username, access_token, business_id
            )
            print(f"Followers: {followers}")
            print(f"Total posts: {posts}")
            print(f"Recent posts (7 days): {recent_posts}")
            
            # Store data in database
            record_id = db.store_metrics(args.username, followers, posts, recent_posts, current_timestamp)
            print(f"Data stored in database with ID: {record_id}")
            
        except Exception as e:
            print(f"Error using API method: {e}")
            print("Try using --method=instaloader or --method=fallback instead")
    
    elif args.method == 'instaloader':
        try:
            print(f"Fetching metrics for @{args.username} using instaloader...")
            followers, posts, recent_posts = get_instagram_followers(args.username)
            print(f"Followers: {followers}")
            print(f"Total posts: {posts}")
            print(f"Recent posts (7 days): {recent_posts}")
            
            # Store data in database
            record_id = db.store_metrics(args.username, followers, posts, recent_posts, current_timestamp)
            print(f"Data stored in database with ID: {record_id}")
            
        except Exception as e:
            print(f"Error using instaloader method: {e}")
            print("Try using --method=api or --method=fallback instead")
    
    elif args.method == 'fallback':
        if not args.url:
            print("Error: URL is required for fallback method")
            print("Please provide --url=https://www.instagram.com/username/")
            sys.exit(1)
            
        try:
            print(f"Fetching metrics for {args.url} using fallback method...")
            followers, posts, recent_posts = get_instagram_followers_fallback(args.url)
            print(f"Followers: {followers}")
            print(f"Total posts: {posts}")
            print(f"Recent posts (7 days): {recent_posts if recent_posts else 'Not available'}")
            
            # Store data in database
            record_id = db.store_metrics(args.username, followers, posts, recent_posts, current_timestamp)
            print(f"Data stored in database with ID: {record_id}")
            
        except Exception as e:
            print(f"Error using fallback method: {e}")
    
    # Display historical data
    print("\nHistorical Data (Last 5 records):")
    historical_data = db.get_historical_data(args.username, 5)
    if historical_data:
        print("Timestamp\t\t\tFollowers\tPosts\tRecent Posts")
        print("-" * 70)
        for record in historical_data:
            followers, posts, recent, timestamp = record
            recent_str = str(recent) if recent is not None else "N/A"
            print(f"{timestamp}\t{followers}\t\t{posts}\t{recent_str}")
    else:
        print("No historical data available yet.")
    
    # Close the database connection
    db.close()

if __name__ == "__main__":
    main()