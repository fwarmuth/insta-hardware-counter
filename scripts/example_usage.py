#!/usr/bin/env python3

from insta_wrapper import InstaWrapper
import os
import logging

def main():
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    logger = logging.getLogger(__name__)
    
    # Path to the CSV file with Instagram credentials
    csv_file_path = os.path.join(os.path.dirname(__file__), 'instagram_credentials.csv')
    
    try:
        # Initialize the wrapper with accounts from CSV file
        logger.info(f"Initializing InstaWrapper with accounts from: {csv_file_path}")
        wrapper = InstaWrapper(csv_file=csv_file_path, selection_method="roundrobin")
        
        # Display how many accounts were loaded
        logger.info(f"Loaded {len(wrapper.accounts)} Instagram accounts from CSV")
        
        # Example: Get profile stats for Instagram's official account
        target_username = "mein.kreis.pinneberg"
        logger.info(f"Fetching profile stats for: {target_username}")
        
        stats = wrapper.get_profile_stats(target_username)
        
        if stats["exists"]:
            logger.info(f"Profile information for {target_username}:")
            logger.info(f"Full name: {stats['full_name']}")
            logger.info(f"Followers: {stats['followers']:,}")
            logger.info(f"Following: {stats['following']:,}")
            logger.info(f"Posts: {stats['posts']:,}")
        else:
            logger.warning(f"Profile {target_username} does not exist")
            
    except Exception as e:
        logger.error(f"Error: {e}", exc_info=True)

if __name__ == "__main__":
    main()