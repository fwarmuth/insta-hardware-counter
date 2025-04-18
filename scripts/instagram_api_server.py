from flask import Flask, jsonify
import time
from datetime import datetime

app = Flask(__name__)

# Placeholder data for Instagram metrics
instagram_data = {
    "username": "test_account",
    "followers_count": 1250,
    "last_updated": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
}

@app.route('/api/instagram/metrics', methods=['GET'])
def get_instagram_metrics():
    """
    Return Instagram metrics in JSON format
    This endpoint is called by the ESP device to get the latest follower count
    """
    # Update the timestamp whenever this endpoint is called
    instagram_data["last_updated"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    return jsonify(instagram_data)


if __name__ == '__main__':
    # Run the server on all interfaces (0.0.0.0) on port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)