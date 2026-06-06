import pyglet
import random

# Initial window dimensions
WIDTH, HEIGHT = 800, 600
window = pyglet.window.Window(
    width=WIDTH, 
    height=HEIGHT, 
    caption="Pyglet Framebuffer with Text Overlay",
    resizable=True
)

def generate_random_color_buffer(w, h):
    """Generates a raw bytearray filled with a single random RGB color."""
    r = random.randint(0, 255)
    g = random.randint(0, 255)
    b = random.randint(0, 255)
    
    raw_data = bytes([r, g, b] * (w * h))
    return pyglet.image.ImageData(w, h, 'RGB', raw_data)

# Create the initial raw pixel background
framebuffer = generate_random_color_buffer(WIDTH, HEIGHT)

# Create a Pyglet Label for text overlay
# We use a distinct color (like black) so it stands out against the background
text_overlay = pyglet.text.Label(
    text="Hello World",
    font_name="Arial",
    font_size=24,
    color=(0, 0, 0, 255),  # RGBA format for text color (Black)
    x=20,                  # Coordinates starting from bottom-left
    y=HEIGHT - 50,
    anchor_x='left', 
    anchor_y='top'
)

def console_print(message):
    """Prints to the terminal and reflects it onto the framebuffer."""
    print(message)                  # Output to standard console
    text_overlay.text = str(message) # Update the window text overlay

@window.event
def on_draw():
    window.clear()
    
    # 1. Draw the raw pixel array first (Background)
    framebuffer.blit(0, 0)
    
    # 2. Draw the text overlay on top of the pixels
    text_overlay.draw()

@window.event
def on_resize(width, height):
    global framebuffer
    # Update framebuffer dimensions
    framebuffer = generate_random_color_buffer(width, height)
    
    # Reposition the text relative to the new window top edge
    text_overlay.y = height - 50

@window.event
def on_mouse_press(x, y, button, modifiers):
    global framebuffer
    framebuffer = generate_random_color_buffer(window.width, window.height)
    
    # Reflect the click action to both console and framebuffer
    console_print(f"Mouse clicked at click point: ({x}, {y})")

@window.event
def on_key_press(symbol, modifiers):
    if symbol == pyglet.window.key.ESCAPE:
        window.close()
    else:
        # Reflect the key press to both console and framebuffer
        # pyglet.window.key.symbol_string converts the key ID to a readable string (e.g., 'A', 'SPACE')
        key_name = pyglet.window.key.symbol_string(symbol)
        console_print(f"Key pressed: {key_name}")

if __name__ == "__main__":
    console_print("Application Started. Click or press keys!")
    pyglet.app.run()