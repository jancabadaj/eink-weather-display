#!/usr/bin/env python3
"""
======= AI GENERATED CODE =======
TTF to C Header Font Converter
Converts TrueType fonts to C header files for the e-ink display project.
Supports both monospace and proportional fonts.

Example usage:
    python ttf_to_header.py -o out.h ~/Downloads/Roboto-BoldCondensed.ttf 58
    python ttf_to_header.py -o out.h ~/Downloads/Roboto-BoldCondensed.ttf 58 --monospace
"""

import argparse
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont


def get_font_metrics(font, height):
    """Get font baseline and metrics."""
    # Use PIL's built-in font metrics
    # getmetrics() returns (ascent, descent) - the maximum vertical extent
    try:
        ascent, descent = font.getmetrics()
    except AttributeError:
        # Fallback for older PIL versions
        # Use a reference character to determine metrics
        bbox = font.getbbox('Ágjpqy|', anchor='ls')  # left-baseline anchor
        ascent = -bbox[1]  # top of bbox (negative, so we negate it)
        descent = bbox[3]  # bottom of bbox (positive)

    # Total font height needed
    total_font_height = ascent + descent

    # If font is larger than target height, scale it down
    if total_font_height > height:
        scale = height / total_font_height
        ascent = int(ascent * scale)
        descent = int(descent * scale)

    # Calculate baseline position
    # We want some margin at top, then the ascent space
    margin_top = max(1, int((height - ascent - descent) / 2))  # Center vertically with margins
    baseline = margin_top + ascent

    # Ensure we don't exceed height
    total_needed = baseline + descent
    if total_needed > height:
        # Adjust baseline down to fit
        baseline = height - descent - 1

    # Ensure baseline is valid
    if baseline < ascent:
        baseline = ascent

    if baseline < 0:
        baseline = int(height * 0.75)  # Fallback to 75% if calculations fail

    return baseline, ascent, descent


def render_character(font, char, height, baseline):
    """Render a single character to a bitmap with proper baseline alignment."""
    # Get font metrics for this character using PIL's built-in method
    # This gives us accurate bounding box without extra whitespace
    try:
        # Get the bounding box of the character relative to baseline
        bbox = font.getbbox(char)  # Returns (left, top, right, bottom)

        if bbox == (0, 0, 0, 0):
            # Empty character (like space) - use font's advance width
            char_width = int(font.getlength(char))
            if char_width <= 0:
                char_width = height // 4
            img = Image.new('1', (char_width, height), 1)
            return img, char_width

        # bbox is relative to the text baseline
        # left is horizontal offset (can be negative for overhanging chars)
        # top is vertical offset above baseline (negative = above baseline)
        # right is right edge
        # bottom is vertical offset below baseline (positive = below baseline)
        left, _, right, _ = bbox

        # Calculate actual character dimensions
        char_width = right - left

        # Ensure minimum width
        if char_width <= 0:
            char_width = height // 4

        # Add minimal padding on each side to prevent clipping
        padding = 1
        char_width += 2 * padding

        # Create the final character image with proper baseline alignment
        img = Image.new('1', (char_width, height), 1)
        draw = ImageDraw.Draw(img)

        # Position character:
        # - Horizontally: offset by padding minus the left bearing
        # - Vertically: place so the baseline is at the calculated position
        #   Since text anchor is at baseline by default, we draw at y=baseline
        #   PIL will handle the top/bottom offsets from there
        char_x = padding - left  # Accounts for left bearing
        char_y = baseline         # This is where the baseline should be in the image

        # Draw the character
        draw.text((char_x, char_y), char, font=font, fill=0, anchor='lb')  # left-baseline anchor

        # Trim horizontal whitespace only, keep full vertical space
        trimmed_bbox = img.getbbox()

        if trimmed_bbox:
            left_trim, _, right_trim, _ = trimmed_bbox

            # Keep minimal horizontal padding
            left_trim = max(0, left_trim - 1)
            right_trim = min(img.width, right_trim + 1)

            # Only trim if we actually reduce the width
            trimmed_width = right_trim - left_trim

            if trimmed_width > 0 and trimmed_width < char_width:
                # Create new image with trimmed width
                trimmed_img = Image.new('1', (trimmed_width, height), 1)
                trimmed_img.paste(img.crop((left_trim, 0, right_trim, height)), (0, 0))
                return trimmed_img, trimmed_width

        return img, char_width

    except Exception as e:
        # Fallback to simple rendering if font.getbbox() fails
        print(f"Warning: Could not get bbox for '{char}': {e}")
        char_width = height // 2
        img = Image.new('1', (char_width, height), 1)
        draw = ImageDraw.Draw(img)
        draw.text((1, baseline), char, font=font, fill=0, anchor='lb')
        return img, char_width


def bitmap_to_bytes(img, width, height):
    """Convert PIL image to byte array in the format expected by the firmware."""
    bytes_per_row = (width + 7) // 8  # Round up to nearest byte
    bitmap_data = []

    for row in range(height):
        row_bytes = [0] * bytes_per_row

        for col in range(width):
            if col < img.width and row < img.height:
                pixel = img.getpixel((col, row))
                # If pixel is black (0), set the bit
                if pixel == 0:
                    byte_idx = col // 8
                    bit_idx = 7 - (col % 8)
                    row_bytes[byte_idx] |= (1 << bit_idx)

        bitmap_data.extend(row_bytes)

    return bitmap_data


def format_bitmap_bytes(bitmap_data, bytes_per_row):
    """Format bitmap bytes as C array with visual comments."""
    lines = []

    for i in range(0, len(bitmap_data), bytes_per_row):
        row_bytes = bitmap_data[i:i + bytes_per_row]

        # Format as hex bytes
        hex_str = ', '.join(f'0x{b:02X}' for b in row_bytes)

        # Create visual representation
        visual = ''
        for byte_val in row_bytes:
            for bit in range(8):
                if byte_val & (0x80 >> bit):
                    visual += '#'
                else:
                    visual += ' '

        lines.append(f'    {hex_str}, // {visual}')

    return '\n'.join(lines)


def convert_ttf_to_header(ttf_path, height, output_path, start_char=32, end_char=126,
                          font_name=None, proportional=True):
    """
    Convert TTF font to C header file.

    Args:
        ttf_path: Path to TTF font file
        height: Font height in pixels
        output_path: Output header file path
        start_char: First ASCII character to include (default: 32 = space)
        end_char: Last ASCII character to include (default: 126 = ~)
        font_name: Name for the font variable (auto-generated if None)
        proportional: Use proportional width (True) or monospace (False)
    """

    # Load the font
    try:
        font = ImageFont.truetype(str(ttf_path), height)
    except Exception as e:
        print(f"Error loading font: {e}", file=sys.stderr)
        sys.exit(1)

    # Generate font name if not provided
    if font_name is None:
        font_name = f"Font{height}_{Path(ttf_path).stem.replace('-', '_').replace(' ', '_')}"

    # Get font metrics for proper baseline alignment
    baseline, ascent, descent = get_font_metrics(font, height)

    print(f"Font metrics: baseline={baseline}, ascent={ascent}, descent={descent}")

    # First pass: render all characters and determine widths
    char_data = {}
    char_widths = {}
    max_width = 0

    for char_code in range(start_char, end_char + 1):
        char = chr(char_code)
        img, char_width = render_character(font, char, height, baseline)
        char_data[char_code] = img
        char_widths[char_code] = char_width
        max_width = max(max_width, char_width - 9) # Subtract some to have tighter fit

    # Determine if we're using proportional or monospace
    if not proportional:
        # Force all characters to max width
        for char_code in char_widths.keys():
            char_widths[char_code] = max_width

    # Generate the header file
    header_content = []
    header_content.append("/**")
    header_content.append(f" * Font generated from {Path(ttf_path).name}")
    header_content.append(f" * Height: {height} pixels")
    header_content.append(f" * Type: {'Proportional' if proportional else 'Monospace'}")
    header_content.append(f" * Characters: ASCII {start_char}-{end_char}")
    header_content.append(f" * Baseline: {baseline} pixels from top")
    header_content.append(" *")
    if proportional:
        header_content.append(" * NOTE: Proportional fonts require updated firmware.")
        header_content.append(" * See README_PROPORTIONAL.md for required changes.")
    header_content.append(" */")
    header_content.append("")
    header_content.append("#pragma once")
    header_content.append("")
    header_content.append('#include "../shape.h"' if not proportional else '#include "../proportionalFont.h"')
    header_content.append("")

    # Generate bitmap data
    if proportional:
        # For proportional fonts, store each character with its actual width
        header_content.append(f"const uint8_t {font_name}_Table[] =")
        header_content.append("{")

        offset = 0
        for char_code in range(start_char, end_char + 1):
            char = chr(char_code)
            img = char_data[char_code]
            char_width = char_widths[char_code]

            # Ensure image is the correct width
            if img.width != char_width:
                padded_img = Image.new('1', (char_width, height), 1)
                padded_img.paste(img, (0, 0))
                img = padded_img

            bytes_per_row = (char_width + 7) // 8
            bitmap_data = bitmap_to_bytes(img, char_width, height)

            char_display = char if char.isprintable() and char not in ['\\', "'", '"'] else f'\\x{char_code:02x}'
            header_content.append(f"    // @{offset} '{char_display}' ({char_width}px wide)")
            header_content.append(format_bitmap_bytes(bitmap_data, bytes_per_row))
            header_content.append("")

            offset += len(bitmap_data)

        header_content.append("};")
        header_content.append("")

        # Generate widths array
        header_content.append(f"const uint16_t {font_name}_Widths[] = {{")
        width_list = [f"{char_widths[c]}" for c in range(start_char, end_char + 1)]

        # Format in rows of 16
        for i in range(0, len(width_list), 16):
            row = width_list[i:i+16]
            header_content.append(f"    {', '.join(row)},")

        header_content.append("};")
        header_content.append("")

        # Generate offsets array
        header_content.append(f"const uint32_t {font_name}_Offsets[] = {{")
        offset = 0
        offsets = []
        for char_code in range(start_char, end_char + 1):
            offsets.append(f"{offset}")
            char_width = char_widths[char_code]
            bytes_per_row = (char_width + 7) // 8
            offset += bytes_per_row * height

        # Format in rows of 8
        for i in range(0, len(offsets), 8):
            row = offsets[i:i+8]
            header_content.append(f"    {', '.join(row)},")

        header_content.append("};")
        header_content.append("")

        # Generate ProportionalFont struct
        header_content.append(f"ProportionalFont {font_name} = {{")
        header_content.append(f"    {font_name}_Table,")
        header_content.append(f"    {font_name}_Widths,")
        header_content.append(f"    {font_name}_Offsets,")
        header_content.append(f"    {max_width}, /* Max width */")
        header_content.append(f"    {height}, /* Height */")
        header_content.append(f"    {start_char}, /* First char */")
        header_content.append("};")

    else:
        # Monospace mode - all characters have same width
        header_content.append(f"const uint8_t {font_name}_Table[] =")
        header_content.append("{")

        bytes_per_row = (max_width + 7) // 8

        for char_code in range(start_char, end_char + 1):
            char = chr(char_code)
            img = char_data[char_code]

            # Pad to max width (center the character)
            if img.width != max_width:
                padded_img = Image.new('1', (max_width, height), 1)
                offset_x = (max_width - img.width) // 2
                padded_img.paste(img, (offset_x, 0))
                img = padded_img

            bitmap_data = bitmap_to_bytes(img, max_width, height)

            offset = (char_code - start_char) * bytes_per_row * height
            char_display = char if char.isprintable() and char not in ['\\', "'", '"'] else f'\\x{char_code:02x}'
            header_content.append(f"    // @{offset} '{char_display}' ({max_width}px wide)")
            header_content.append(format_bitmap_bytes(bitmap_data, bytes_per_row))
            header_content.append("")

        header_content.append("};")
        header_content.append("")
        header_content.append(f"Shape {font_name} = {{")
        header_content.append(f"    {font_name}_Table,")
        header_content.append(f"    {max_width}, /* Width */")
        header_content.append(f"    {height}, /* Height */")
        header_content.append("};")

    header_content.append("")

    # Write to file
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, 'w') as f:
        f.write('\n'.join(header_content))

    # Calculate total size
    total_bytes = 0
    for char_code in range(start_char, end_char + 1):
        char_width = char_widths[char_code] if proportional else max_width
        bytes_per_row = (char_width + 7) // 8
        total_bytes += bytes_per_row * height

    print(f"✓ Font converted successfully!")
    print(f"  Output: {output_path}")
    print(f"  Font name: {font_name}")
    print(f"  Type: {'Proportional' if proportional else 'Monospace'}")
    print(f"  Height: {height}px, Max width: {max_width}px")
    print(f"  Characters: {end_char - start_char + 1} ({chr(start_char)} to {chr(end_char)})")
    print(f"  Bitmap data: {total_bytes} bytes")
    if proportional:
        print(f"  Width table: {(end_char - start_char + 1) * 2} bytes")
        print(f"  Offset table: {(end_char - start_char + 1) * 4} bytes")


def main():
    parser = argparse.ArgumentParser(
        description='Convert TrueType fonts to C header files for e-ink display',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Convert font to 24px height (proportional)
  %(prog)s font.ttf 24 -o output.h

  # Convert to monospace
  %(prog)s font.ttf 64 -o numbers.h --monospace

  # Custom character range (numbers only, proportional)
  %(prog)s font.ttf 64 -o numbers.h --start 48 --end 57

  # Custom font name
  %(prog)s font.ttf 48 -o big.h --name BigFont48
        """
    )

    parser.add_argument('ttf_file', type=Path, help='Path to TTF font file')
    parser.add_argument('height', type=int, help='Font height in pixels')
    parser.add_argument('-o', '--output', type=Path, required=True,
                       help='Output header file path')
    parser.add_argument('--start', type=int, default=32,
                       help='First ASCII character code (default: 32 = space)')
    parser.add_argument('--end', type=int, default=126,
                       help='Last ASCII character code (default: 126 = ~)')
    parser.add_argument('--name', type=str,
                       help='Font variable name (auto-generated if not specified)')
    parser.add_argument('--monospace', action='store_true',
                       help='Generate monospace font (compatible with current firmware)')

    args = parser.parse_args()

    # Validate inputs
    if not args.ttf_file.exists():
        print(f"Error: Font file not found: {args.ttf_file}", file=sys.stderr)
        sys.exit(1)

    if args.height < 8 or args.height > 256:
        print(f"Error: Height must be between 8 and 256 pixels", file=sys.stderr)
        sys.exit(1)

    if args.start < 0 or args.end > 127 or args.start > args.end:
        print(f"Error: Invalid character range (start={args.start}, end={args.end})",
              file=sys.stderr)
        sys.exit(1)

    # Convert the font
    convert_ttf_to_header(
        args.ttf_file,
        args.height,
        args.output,
        args.start,
        args.end,
        args.name,
        proportional=not args.monospace
    )


if __name__ == '__main__':
    main()
