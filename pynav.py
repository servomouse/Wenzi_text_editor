import pyglet   # pip install pyglet
import random

# Window dimensions
WIDTH, HEIGHT = 800, 600
window = pyglet.window.Window(width=WIDTH, height=HEIGHT, caption="Pyglet Framebuffer")

def generate_random_color_buffer(w, h):
    """Generates a raw bytearray filled with a single random RGB color."""
    r = random.randint(0, 255)
    g = random.randint(0, 255)
    b = random.randint(0, 255)
    
    # Create a flat array of RGB values for every single pixel
    # Multiplying a 3-byte list by the total number of pixels
    raw_data = bytes([r, g, b] * (w * h))
    
    # Wrap the raw bytes into a Pyglet ImageData object
    # 'RGB' tells it the format, and pitch is the number of bytes per row (w * 3)
    return pyglet.image.ImageData(w, h, 'RGB', raw_data)

# Initialize our framebuffer with a starting color
framebuffer = generate_random_color_buffer(WIDTH, HEIGHT)

@window.event
def on_draw():
    window.clear()
    # Blit the raw pixel buffer directly to the screen at coordinates (0, 0)
    framebuffer.blit(0, 0)

@window.event
def on_mouse_press(x, y, button, modifiers):
    global framebuffer
    print(f"Mouse clicked at ({x}, {y}). Generating new random color buffer!")
    # Regenerate the raw pixel data on click
    framebuffer = generate_random_color_buffer(WIDTH, HEIGHT)

@window.event
def on_key_press(symbol, modifiers):
    print(f"Key pressed: {symbol}")
    # Close window cleanly on ESC
    if symbol == pyglet.window.key.ESCAPE:
        window.close()

if __name__ == "__main__":
    pyglet.app.run()