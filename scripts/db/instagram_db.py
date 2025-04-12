#!/usr/bin/env python3
import sqlite3
import os
import datetime

class InstagramDatabase:
    def __init__(self, db_path='data/instagram_metrics.db'):
        """
        Initialize the database connection.
        
        Args:
            db_path (str): Path to the SQLite database file
        """
        # Create the directory if it doesn't exist
        os.makedirs(os.path.dirname(db_path), exist_ok=True)
        
        # Initialize the database
        self.db_path = db_path
        self.conn = sqlite3.connect(db_path)
        self.cursor = self.conn.cursor()
        
        # Create tables if they don't exist
        self._create_tables()
    
    def _create_tables(self):
        """Create necessary tables if they don't exist."""
        self.cursor.execute('''
        CREATE TABLE IF NOT EXISTS instagram_metrics (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            followers_count INTEGER,
            posts_count INTEGER,
            recent_posts_count INTEGER,
            collection_date TEXT NOT NULL
        )
        ''')
        self.conn.commit()
    
    def store_metrics(self, username, followers_count, posts_count, recent_posts_count, timestamp=None):
        """
        Store Instagram metrics in the database.
        
        Args:
            username (str): Instagram username
            followers_count (int): Number of followers
            posts_count (int): Total number of posts
            recent_posts_count (int): Number of posts in the last 7 days
            timestamp (str, optional): Timestamp of collection in ISO format (YYYY-MM-DD HH:MM:SS).
                                     Defaults to current timestamp.
        
        Returns:
            int: ID of the inserted record
        """
        # Use current timestamp if not provided
        if timestamp is None:
            timestamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        self.cursor.execute('''
        INSERT INTO instagram_metrics 
        (username, followers_count, posts_count, recent_posts_count, collection_date)
        VALUES (?, ?, ?, ?, ?)
        ''', (username, followers_count, posts_count, recent_posts_count, timestamp))
        
        self.conn.commit()
        return self.cursor.lastrowid
    
    def get_latest_metrics(self, username):
        """
        Get the most recent metrics for a given username.
        
        Args:
            username (str): Instagram username
            
        Returns:
            tuple: (followers_count, posts_count, recent_posts_count, collection_date)
                   or None if no records found
        """
        self.cursor.execute('''
        SELECT followers_count, posts_count, recent_posts_count, collection_date
        FROM instagram_metrics
        WHERE username = ?
        ORDER BY collection_date DESC
        LIMIT 1
        ''', (username,))
        
        result = self.cursor.fetchone()
        return result
    
    def get_historical_data(self, username, limit=30):
        """
        Get historical metrics for trend analysis.
        
        Args:
            username (str): Instagram username
            limit (int): Maximum number of records to retrieve
            
        Returns:
            list: List of tuples containing
                 (followers_count, posts_count, recent_posts_count, collection_date)
        """
        self.cursor.execute('''
        SELECT followers_count, posts_count, recent_posts_count, collection_date
        FROM instagram_metrics
        WHERE username = ?
        ORDER BY collection_date DESC
        LIMIT ?
        ''', (username, limit))
        
        return self.cursor.fetchall()
    
    def close(self):
        """Close the database connection."""
        if self.conn:
            self.conn.close()