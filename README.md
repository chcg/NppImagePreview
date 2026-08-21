# NppImagePreview

**Notepad++ Image Preview Plugin**

NppImagePreview is a Notepad++ plugin that lets you preview images referenced in your source files without leaving Notepad++.

Hover over an image path in an HTML, CSS, PHP, or JavaScript file to display a thumbnail preview popup.

You can also manually trigger the preview using **Preview Image at Cursor**.

## Features

- Image preview on hover
- Manual image preview at the cursor
- Thumbnail popup
- Designed for HTML, CSS, PHP, and JavaScript files
- Resolves image paths from the current document context
- Lightweight native Windows plugin
- Built with C++ and the Notepad++ plugin SDK

## Supported Image Formats

- PNG
- JPG / JPEG
- WebP
- GIF
- SVG
- BMP
- ICO
- AVIF
- TIFF

## Usage

### Hover Preview

1. Open an HTML, CSS, PHP, or JavaScript file in Notepad++.
2. Place the mouse over an image path.
3. NppImagePreview detects the image reference.
4. A thumbnail preview popup is displayed.

### Preview Image at Cursor

You can also manually trigger the preview for an image path at the current cursor position using:

**Preview Image at Cursor**

## Example

For an HTML document containing:

```html
<img src="images/example.png">
```

hover over:

```text
images/example.png
```

to preview the image.

## Installation

### Manual Installation

1. Download the appropriate NppImagePreview release package.
2. Close Notepad++.
3. Install the plugin DLL into the appropriate Notepad++ plugins directory.
4. Start Notepad++ again.
5. Open an HTML, CSS, PHP, or JavaScript document and try hovering over an image path.

> Installation instructions may vary depending on the Notepad++ architecture and release package.

## Building from Source

### Requirements

- Windows
- Visual Studio
- C++ development tools
- Windows SDK
- Notepad++ plugin SDK

### Build

Open:

```text
NppImagePreview.slnx
```

in Visual Studio.

Select:

```text
Configuration: Release
Platform: x64
```

Then use:

**Build → Rebuild Solution**

The generated plugin DLL will be placed in the Visual Studio build output directory.

## Project Structure

```text
NppImagePreview/
├── include/
├── sdk/
├── src/
├── NppImagePreview.slnx
├── NppImagePreview.vcxproj
├── NppImagePreview.vcxproj.filters
├── .gitattributes
└── .gitignore
```

## Version

**v1.0.0**

Initial public release.

## Future Development

Future versions may introduce additional image-preview functionality and UI improvements.

## Contributing

Bug reports, feature suggestions, and contributions are welcome.

Please open an issue or pull request on GitHub.

## License

See [LICENSE](LICENSE) for license information.

## Disclaimer

NppImagePreview is an independent community project and is not affiliated with or endorsed by Notepad++.
