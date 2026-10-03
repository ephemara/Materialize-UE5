#!/usr/bin/env python3
"""
Generate toolbar icons for the Materialize plugin.
Creates 40x40 and 16x16 pixel icons with a stylized "M" design.
"""

from PIL import Image, ImageDraw, ImageFont
import os

def create_materialize_icon(size, output_path):
    """
    Create a Materialize icon with a stylized "M" and material sphere motif.
    
    Args:
        size: Icon size (40 or 16)
        output_path: Path to save the PNG file
    """
    # Create image with transparency
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Color scheme: Professional blue-purple gradient
    bg_color = (45, 55, 75, 255)  # Dark blue-gray background
    accent_color = (100, 180, 255, 255)  # Bright blue accent
    highlight_color = (180, 120, 255, 255)  # Purple highlight
    
    # Draw rounded rectangle background
    margin = 2 if size == 40 else 1
    draw.rounded_rectangle(
        [(margin, margin), (size - margin, size - margin)],
        radius=size // 8,
        fill=bg_color
    )
    
    if size == 40:
        # Draw stylized "M" for 40x40 icon
        # M shape with material texture motif
        m_color = accent_color
        
        # Left vertical bar
        draw.rectangle([(8, 12), (12, 32)], fill=m_color)
        
        # Right vertical bar
        draw.rectangle([(28, 12), (32, 32)], fill=m_color)
        
        # Left diagonal
        draw.polygon([(12, 12), (18, 20), (18, 16), (12, 12)], fill=m_color)
        
        # Right diagonal
        draw.polygon([(28, 12), (22, 20), (22, 16), (28, 12)], fill=m_color)
        
        # Center vertical (short)
        draw.rectangle([(18, 16), (22, 26)], fill=m_color)
        
        # Add small material sphere in bottom right
        sphere_center = (30, 30)
        sphere_radius = 4
        
        # Sphere gradient effect
        for r in range(sphere_radius, 0, -1):
            alpha = int(255 * (r / sphere_radius))
            color = tuple(list(highlight_color[:3]) + [alpha])
            draw.ellipse(
                [
                    (sphere_center[0] - r, sphere_center[1] - r),
                    (sphere_center[0] + r, sphere_center[1] + r)
                ],
                fill=color
            )
        
        # Highlight on sphere
        draw.ellipse(
            [(sphere_center[0] - 1, sphere_center[1] - 2),
             (sphere_center[0] + 1, sphere_center[1])],
            fill=(255, 255, 255, 200)
        )
    
    else:  # size == 16
        # Simplified "M" for 16x16 icon
        m_color = accent_color
        
        # Left vertical bar
        draw.rectangle([(3, 4), (5, 13)], fill=m_color)
        
        # Right vertical bar
        draw.rectangle([(11, 4), (13, 13)], fill=m_color)
        
        # Left diagonal
        draw.polygon([(5, 4), (7, 7), (7, 5), (5, 4)], fill=m_color)
        
        # Right diagonal
        draw.polygon([(11, 4), (9, 7), (9, 5), (11, 4)], fill=m_color)
        
        # Center vertical (short)
        draw.rectangle([(7, 5), (9, 10)], fill=m_color)
    
    # Save the image
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    img.save(output_path, 'PNG')
    print(f"Created {size}x{size} icon: {output_path}")

def main():
    """Generate both icon sizes."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    plugin_root = os.path.dirname(script_dir)
    icons_dir = os.path.join(plugin_root, 'Content', 'Icons')
    
    # Create 40x40 toolbar icon
    create_materialize_icon(40, os.path.join(icons_dir, 'KSample_Icon_40x.png'))
    
    # Create 16x16 menu icon
    create_materialize_icon(16, os.path.join(icons_dir, 'KSample_Icon_16x.png'))
    
    print("\nIcon generation complete!")
    print(f"Icons saved to: {icons_dir}")

if __name__ == '__main__':
    main()
