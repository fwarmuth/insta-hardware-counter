#!/usr/bin/env python3
import os
import sys
import time
import datetime
import logging
from flask import Flask, jsonify
from typing import Dict, Any

# Add the scripts directory to the path to import our modules
current_dir = os.path.dirname(os.path.realpath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(current_dir)

# Import the functions and database module
from get_followers import get_instagram_metrics
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
    
    # Get login credentials from environment variables
    login_username = os.environ.get('INSTAGRAM_USERNAME')
    login_password = os.environ.get('INSTAGRAM_PASSWORD')
    
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
            metrics, record_id = get_instagram_metrics(
                username, 
                store_in_db=True, 
                db_path=db_path,
                login_username=login_username,
                login_password=login_password
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