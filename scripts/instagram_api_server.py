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

@app.route('/api/instagram/update', methods=['GET'])
def update_metrics():
    """
    Test endpoint to simulate updating the follower count
    In a real application, you would update this from your Instagram API script
    """
    # Increase the follower count by a random number between 1 and 10
    instagram_data["followers_count"] += 1
    instagram_data["last_updated"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    return jsonify({"status": "success", "new_count": instagram_data["followers_count"]})

@app.route('/')
def index():
    """
    Simple homepage with links to the API endpoints
    """
    return """
    <html>
        <head>
            <title>Instagram Metrics API</title>
            <style>
                body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
                code { background-color: #f4f4f4; padding: 2px 5px; border-radius: 3px; }
            </style>
        </head>
        <body>
            <h1>Instagram Metrics API</h1>
            <p>This API serves Instagram metrics data to the ESP device.</p>
            
            <h2>Available Endpoints:</h2>
            <ul>
                <li><a href="/api/instagram/metrics">/api/instagram/metrics</a> - Get the latest metrics</li>
                <li><a href="/api/instagram/update">/api/instagram/update</a> - Increase follower count by 1 (for testing)</li>
            </ul>
            
            <h2>Current Data:</h2>
            <pre>{}</pre>
        </body>
    </html>
    """.format(instagram_data)

if __name__ == '__main__':
    # Run the server on all interfaces (0.0.0.0) on port 5000
    app.run(host='0.0.0.0', port=5000, debug=True)