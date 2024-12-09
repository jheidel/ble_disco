import os
import sys
import asyncio
import signal
import json
from bleak import BleakClient
from aiohttp import web

ADDRESS = "C2:AD:B2:39:23:C9"  # Replace with your device's address
SPIN_UUID = "94cc0002-e5cb-483a-bb0c-58e34568674c"  # Replace with your characteristic UUID


async def handle_spin(request):
    """
    Handle the /spin endpoint, parsing the 'seconds' from the JSON payload and writing it to the BLE characteristic.
    """
    client = request.app['client']  # Access the BleakClient from the app's state

    try:
        # Get the JSON payload
        data = await request.json()

        # Extract the 'seconds' value
        seconds = data.get("seconds")
        if seconds is None:
            return web.json_response({"error": "Missing 'seconds' in the payload"}, status=400)

        try:
            # Convert to an integer and validate it
            seconds_value = int(seconds)
            if not (0 <= seconds_value <= 255):
                return web.json_response({"error": "'seconds' value out of range (0-255)"}, status=400)

            # Log the value being written to BLE
            print(f"Writing {seconds_value} to BLE characteristic {SPIN_UUID}...", file=sys.stderr)

            # Write the value as a single byte to BLE
            await client.write_gatt_char(
                SPIN_UUID, bytearray([seconds_value]), response=False
            )

            return web.json_response({"status": "success", "seconds": seconds_value})

        except ValueError:
            return web.json_response({"error": "'seconds' must be an integer"}, status=400)

    except Exception as e:
        return web.json_response({"error": f"Internal server error: {str(e)}"}, status=500)


async def start_server(client):
    app = web.Application()
    app['client'] = client  # Store the client in the app's state
    app.router.add_post('/spin', handle_spin)

    print("Starting HTTP server on port 4747...", file=sys.stderr)
    # Await the web.run_app coroutine to properly start the server
    await web._run_app(app, host='0.0.0.0', port=4747)


def disconnected(w):
  print("disconnected!", file=sys.stderr)
  exit(1)


async def main(address):
  print(f"Attempting to connect to the BLE device at address {address}...", file=sys.stderr)

  async with BleakClient(address, timeout=60.0, disconnected_callback=disconnected) as client:
      print(f"Connected to BLE device at address {address}: {client.is_connected}", file=sys.stderr)

      # Start the HTTP server with the BLE client
      await start_server(client)


if __name__ == "__main__":
    # Ensure the asyncio event loop is running
    asyncio.run(main(ADDRESS))

