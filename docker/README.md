# Instagram API Docker Setup

This directory contains Docker configuration files for running the Instagram API service.

## Prerequisites

- Docker
- Docker Compose

## Building and Running the Container

From the project root directory, run:

```bash
cd docker
docker-compose up -d
```

This will:
1. Build the Docker image using the Dockerfile
2. Start the container in detached mode
3. Map port 5000 on your host to port 5000 in the container
4. Mount the data directory for persistent storage

## Checking Logs

```bash
docker-compose logs -f
```

## Stopping the Container

```bash
docker-compose down
```

## Configuration

You can modify the following environment variables in docker-compose.yml:

- `PORT`: The port the API server listens on (default: 5000)
- `HOST`: The host interface to bind to (default: 0.0.0.0, all interfaces)

## Volume Mounts

The data directory is mounted as a volume to ensure persistence of the SQLite database.