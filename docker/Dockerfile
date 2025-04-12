FROM python:3.9-slim

WORKDIR /app

# Copy requirements file and install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy only the application code (not the data)
COPY scripts/ /app/scripts/

# Set the environment variables
ENV PORT=5000
ENV HOST=0.0.0.0

# Expose the API port
EXPOSE 5000

# Run the API server
CMD ["python", "scripts/instagram_api.py"]