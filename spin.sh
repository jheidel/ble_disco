#!/bin/bash

# Ensure that an argument is provided
if [ -z "$1" ]; then
  echo "Usage: $0 <seconds>"
  exit 1
fi

# Get the value from the first argument
SECONDS=$1

# Validate that the input is a valid integer
if ! [[ "$SECONDS" =~ ^[0-9]+$ ]]; then
  echo "Error: '$SECONDS' is not a valid integer."
  exit 1
fi

# Ensure the value is between 0 and 255
if [ "$SECONDS" -lt 0 ] || [ "$SECONDS" -gt 255 ]; then
  echo "Error: '$SECONDS' must be between 0 and 255."
  exit 1
fi

# Post the payload to the server
curl -X POST http://localhost:4747/spin \
     -H "Content-Type: application/json" \
     -d "{\"seconds\": \"$SECONDS\"}"

echo "Request sent to /spin with seconds=$SECONDS"
