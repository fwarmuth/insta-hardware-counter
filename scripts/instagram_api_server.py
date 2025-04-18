from flask import Flask, jsonify, request, g
import time
from datetime import datetime
import os
import sys

# Add the current directory to path to ensure imports work
current_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(current_dir)

# Import the InstaWrapper and Instagram database
from insta_wrapper import InstaWrapper
from db.instagram_db import InstagramDatabase

app = Flask(__name__)

# Initialize the Instagram wrapper with credentials from CSV
CREDENTIALS_FILE = os.path.join(current_dir, "instagram_credentials.csv")
insta_api = InstaWrapper(csv_file=CREDENTIALS_FILE)

# Define the database path
DB_PATH = os.path.join(os.path.dirname(current_dir), "data", "instagram_metrics.db")

def get_db():
    """Get a new database connection for the current request."""
    if 'db' not in g:
        g.db = InstagramDatabase(db_path=DB_PATH)
    return g.db

@app.teardown_appcontext
def close_db(e=None):
    """Close the database connection at the end of the request."""
    db = g.pop('db', None)
    if db is not None:
        db.close()

@app.route('/api/instagram/metrics', methods=['GET'])
def get_instagram_metrics():
    """
    Return Instagram metrics in JSON format
    This endpoint is called by the ESP device to get the latest follower count
    """
    app.logger.info(f"Received request for Instagram metrics at {datetime.now()} and arguments: {request.args}")
    username = request.args.get('username', 'mein.kreis.pinneberg')
    
    try:
        # Get a thread-safe database connection for this request
        db = get_db()
        
        # Try to get the latest metrics from the database first
        latest_metrics = db.get_latest_metrics(username)
        
        # If we have recent data (less than 1 hour old), use it
        if latest_metrics:
            followers_count, posts_count, recent_posts_count, collection_date = latest_metrics
            last_updated = collection_date
            
            # Check if data is older than 1 hour and needs refresh
            last_update_time = datetime.strptime(collection_date, '%Y-%m-%d %H:%M:%S')
            time_diff = (datetime.now() - last_update_time).total_seconds() / 3600
            
            if time_diff > 1:
                # Data is older than 1 hour, fetch new data
                refresh_data = True
                app.logger.info(f"Data is {time_diff:.2f} hours old, refreshing...")
            else:
                refresh_data = False
                app.logger.info(f"Using cached data, last updated at {last_updated}")
        else:
            # No data in database, need to fetch
            refresh_data = True
        
        # If we need fresh data, fetch it from Instagram
        if refresh_data:
            app.logger.info(f"Fetching fresh data for {username} from Instagram API...")
            stats = insta_api.get_profile_stats(username)
            
            if stats["exists"]:
                followers_count = stats["followers"]
                posts_count = stats["posts"]
                # We don't have recent posts count from the API
                recent_posts_count = 0
                
                # Store the fresh data in the database
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                db.store_metrics(
                    username=username,
                    followers_count=followers_count,
                    posts_count=posts_count,
                    recent_posts_count=recent_posts_count,
                    timestamp=timestamp
                )
                last_updated = timestamp
            else:
                # If profile doesn't exist and we don't have cached data
                if not latest_metrics:
                    return jsonify({
                        "error": "Profile not found",
                        "username": username
                    }), 404
        
        # Return formatted response
        return jsonify({
            "username": username,
            "followers_count": followers_count,
            "posts_count": posts_count,
            "recent_posts_count": recent_posts_count,
            "last_updated": last_updated
        })
        
    except Exception as e:
        app.logger.error(f"Error fetching Instagram metrics: {e}")
        return jsonify({
            "error": str(e),
            "username": username
        }), 500


if __name__ == '__main__':
    # Run the server on all interfaces (0.0.0.0) on port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)