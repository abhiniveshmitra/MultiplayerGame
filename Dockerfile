FROM debian:bullseye-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy source files
COPY . .

# Build the application
RUN g++ -std=c++11 -pthread server.cpp -o server

# Expose the port
EXPOSE 8080/udp

# Run the server
CMD ["./server"]
