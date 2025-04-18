import instaloader
import random
from typing import List, Dict, Tuple, Optional, Union
import time
import logging
import csv
import os.path


class InstaWrapper:
    """
    A wrapper for instaloader that manages multiple Instagram accounts 
    and rotates through them to fetch profiles.
    """
    
    def __init__(self, accounts=None, csv_file=None, selection_method: str = "roundrobin"):
        """
        Initialize the InstaWrapper with a list of Instagram accounts.
        
        Args:
            accounts: List of dictionaries with keys 'username' and 'password'
            csv_file: Path to a CSV file containing Instagram accounts
            selection_method: Method to select the next account ('roundrobin' or 'random')
        """
        # Load accounts from CSV file if provided
        if csv_file:
            self.accounts = self.load_accounts_from_csv(csv_file)
        else:
            self.accounts = accounts or []
            
        if not self.accounts:
            raise ValueError("At least one account must be provided (either as list or CSV file)")
        
        self.selection_method = selection_method
        if selection_method not in ["roundrobin", "random"]:
            raise ValueError("Selection method must be 'roundrobin' or 'random'")
        
        self.current_index = 0
        self.loaders = {}  # Cache for instaloader instances
        self.login_timestamps = {}  # Track when each account was last used
        
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger("InstaWrapper")
    
    @staticmethod
    def load_accounts_from_csv(csv_file_path: str) -> List[Dict[str, str]]:
        """
        Load Instagram accounts from a CSV file.
        
        Expected format:
        email,user:password
        
        Args:
            csv_file_path: Path to the CSV file
            
        Returns:
            List of dictionaries with 'username' and 'password' keys
        """
        if not os.path.exists(csv_file_path):
            raise FileNotFoundError(f"CSV file not found: {csv_file_path}")
        
        accounts = []
        with open(csv_file_path, 'r', encoding='utf-8') as csvfile:
            reader = csv.reader(csvfile)
            # Skip header row
            next(reader, None)
            
            for row in reader:
                if len(row) >= 2 and ":" in row[1]:
                    email = row[0].strip()
                    user_pass = row[1].strip()
                    username, password = user_pass.split(':', 1)  # Split only on the first colon
                    
                    accounts.append({
                        'email': email,
                        'username': username.strip(),
                        'password': password.strip()
                    })
                else:
                    logging.warning(f"Skipping invalid row in CSV: {row}")
        
        if not accounts:
            raise ValueError(f"No valid accounts found in CSV file: {csv_file_path}")
            
        return accounts
    
    def _get_next_account(self) -> Dict[str, str]:
        """
        Get the next account based on the selection method.
        
        Returns:
            Dict with 'username' and 'password'
        """
        if self.selection_method == "random":
            return random.choice(self.accounts)
        else:  # roundrobin
            account = self.accounts[self.current_index]
            self.current_index = (self.current_index + 1) % len(self.accounts)
            return account
    
    def _get_loader(self, account: Dict[str, str]) -> instaloader.Instaloader:
        """
        Get or create an authenticated instaloader instance for the given account.
        
        Args:
            account: Dict with 'username' and 'password'
            
        Returns:
            Authenticated instaloader instance
        """
        username = account['username']
        
        # Check if we already have a loader for this account
        if username in self.loaders:
            # Check if we need to re-login (e.g., after 24 hours)
            last_login = self.login_timestamps.get(username, 0)
            if time.time() - last_login < 86400:  # 24 hours in seconds
                return self.loaders[username]
        
        # Create a new loader and login
        try:
            loader = instaloader.Instaloader()
            self.logger.info(f"Logging in with account: {username}")
            loader.login(username, account['password'])
            self.loaders[username] = loader
            self.login_timestamps[username] = time.time()
            return loader
        except instaloader.exceptions.ConnectionException:
            self.logger.error(f"Connection error while logging in with account: {username}")
            raise
        except instaloader.exceptions.BadCredentialsException:
            self.logger.error(f"Bad credentials for account: {username}")
            raise
        except Exception as e:
            self.logger.error(f"Unexpected error for account {username}: {e}")
            raise
    
    def get_profile(self, username: str) -> Optional[instaloader.Profile]:
        """
        Get an Instagram profile using one of the accounts.
        
        Args:
            username: Instagram username of the profile to fetch
            
        Returns:
            Profile object or None if profile not found
        """
        # Try with different accounts if needed
        for attempt in range(min(3, len(self.accounts))):
            account = self._get_next_account()
            try:
                loader = self._get_loader(account)
                self.logger.info(f"Fetching profile for {username} with account {account['username']}")
                profile = instaloader.Profile.from_username(loader.context, username)
                self.logger.info(f"Profile {username} fetched successfully")
                return profile
            except instaloader.exceptions.ProfileNotExistsException:
                self.logger.warning(f"Profile {username} does not exist")
                return None
            except instaloader.exceptions.TooManyRequestsException:
                self.logger.warning(f"Rate limited with account {account['username']}, trying another account")
                # Remove this loader so we try a different account next time
                if account['username'] in self.loaders:
                    del self.loaders[account['username']]
                if attempt == len(self.accounts) - 1:
                    self.logger.error("All accounts are rate limited")
                    raise
            except Exception as e:
                self.logger.error(f"Error fetching profile {username}: {e}")
                if attempt == len(self.accounts) - 1:
                    raise
        
        return None
    
    def get_follower_count(self, username: str) -> Optional[int]:
        """
        Get the follower count for an Instagram profile.
        
        Args:
            username: Instagram username
            
        Returns:
            Number of followers or None if profile not found
        """
        profile = self.get_profile(username)
        return profile.followers if profile else None
    
    def get_following_count(self, username: str) -> Optional[int]:
        """
        Get the following count for an Instagram profile.
        
        Args:
            username: Instagram username
            
        Returns:
            Number of accounts following or None if profile not found
        """
        profile = self.get_profile(username)
        return profile.followees if profile else None
    
    def get_media_count(self, username: str) -> Optional[int]:
        """
        Get the number of posts for an Instagram profile.
        
        Args:
            username: Instagram username
            
        Returns:
            Number of posts or None if profile not found
        """
        profile = self.get_profile(username)
        return profile.mediacount if profile else None
    
    def get_profile_stats(self, username: str) -> Dict[str, Optional[Union[int, str]]]:
        """
        Get comprehensive stats for an Instagram profile.
        
        Args:
            username: Instagram username
            
        Returns:
            Dictionary with profile statistics
        """
        profile = self.get_profile(username)
        if not profile:
            return {
                "exists": False,
                "followers": None,
                "following": None,
                "posts": None,
                "biography": None,
                "full_name": None
            }
        
        return {
            "exists": True,
            "followers": profile.followers,
            "following": profile.followees,
            "posts": profile.mediacount,
            "biography": profile.biography,
            "full_name": profile.full_name
        }
