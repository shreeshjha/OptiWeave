# OptiWeave Web Dashboard

A standalone HTML dashboard for visualizing OptiWeave performance analysis and optimization suggestions.

## Features

- **Performance Hotspots**: Flame graph visualization of code bottlenecks
- **Operation Statistics**: Pie chart breakdown of operation types
- **Optimization Suggestions**: Detailed recommendations with code examples
- **Timeline View**: Execution phase visualization
- **Responsive Design**: Works on desktop and mobile devices

## Quick Start

### 1. Generate Data

Run your OptiWeave-instrumented program with JSON export enabled:

```bash
# Transform and compile your code
./optiweave --compile --enable-stats --enable-timing -o myprogram mycode.cpp

# Run with JSON export
env OPTIWEAVE_STATS=1 \
    OPTIWEAVE_TIMING=1 \
    OPTIWEAVE_HOTSPOTS=1 \
    OPTIWEAVE_SUGGESTIONS=1 \
    OPTIWEAVE_JSON_EXPORT=1 \
    OPTIWEAVE_JSON_FILE=dashboard_data.json \
    ./myprogram
```

This will generate `dashboard_data.json` in the current directory.

### 2. View Dashboard

#### Option A: Using Mock Data (No server needed)

Simply open `dashboard.html` in your web browser:

```bash
open web/dashboard.html
```

The dashboard will display mock data for demonstration purposes.

#### Option B: Using Real Data (Requires local server)

Due to browser security restrictions (CORS), loading JSON files requires a local web server:

```bash
# Using Python 3
cd web
python3 -m http.server 8000

# Using Python 2
cd web
python -m SimpleHTTPServer 8000

# Using Node.js
cd web
npx http-server -p 8000
```

Then:
1. Copy your `dashboard_data.json` to the `web/` directory
2. Open http://localhost:8000/dashboard_dynamic.html in your browser

The dashboard will automatically load data from `dashboard_data.json` if available, otherwise it falls back to mock data.

## Environment Variables

| Variable | Description | Values | Default |
|----------|-------------|--------|---------|
| `OPTIWEAVE_STATS` | Enable operation statistics | 1, true | disabled |
| `OPTIWEAVE_TIMING` | Enable timing profiling | 1, true | disabled |
| `OPTIWEAVE_HOTSPOTS` | Enable hotspot tracking | 1, true | disabled |
| `OPTIWEAVE_SUGGESTIONS` | Enable optimization suggestions | 1, true | disabled |
| `OPTIWEAVE_JSON_EXPORT` | Enable JSON export | 1, true | disabled |
| `OPTIWEAVE_JSON_FILE` | JSON output filename | string | `optiweave_dashboard.json` |

## Dashboard Structure

### Summary Cards
- **Total Operations**: Total number of tracked operations
- **Runtime**: Total execution time in milliseconds
- **Issues Found**: Number of optimization opportunities detected
- **Potential Speedup**: Estimated combined speedup from all suggestions

### Hotspots
Flame graph showing the top performance bottlenecks:
- Function name and source location
- Time spent (absolute and percentage)
- Number of operations

### Operations Breakdown
Pie chart showing distribution of operation types:
- Array accesses
- Arithmetic operations (add, mul, div, etc.)
- Comparisons
- Assignments

### Optimization Suggestions
Categorized by severity (HIGH, MEDIUM, LOW):
- Pattern detected
- Current code snippet
- Optimized code snippet
- Estimated speedup range
- Requirements and rationale

## Files

- `dashboard.html` - Static dashboard with mock data (works offline)
- `dashboard_dynamic.html` - Dynamic dashboard that loads JSON data
- `load_data.js` - Data loading utility (fetches JSON or falls back to mock data)
- `README.md` - This file

## Technology Stack

- **D3.js v7**: Data visualizations (charts, graphs)
- **Tailwind CSS**: Responsive styling
- **Vanilla JavaScript**: No framework dependencies
- **Pure HTML/CSS**: No build process required

## Browser Compatibility

- Chrome/Edge 90+
- Firefox 88+
- Safari 14+

## Troubleshooting

### "No data displayed" or "Using mock data"

**Cause**: JSON file not found or contains empty data

**Solutions**:
1. Ensure `dashboard_data.json` exists in the correct location
2. Check that all required environment variables are set when running your program
3. Verify OptiWeave instrumentation was enabled during compilation
4. Use a local web server (see "Option B" above)

### "CORS error" in browser console

**Cause**: Loading JSON files from `file://` protocol is blocked by browsers

**Solution**: Use a local web server (see "Option B" above)

### Visualizations not rendering

**Cause**: D3.js or Tailwind CSS failed to load from CDN

**Solutions**:
1. Check internet connection
2. Open browser developer console to see specific errors
3. Try refreshing the page

## Customization

To customize the dashboard appearance, edit the `<style>` section in `dashboard.html`:

- Colors: Modify gradient colors in `body` background
- Glass effect: Adjust `backdrop-filter` and opacity in `.glass` class
- Fonts: Change `font-family` in `body` or specific elements
- Charts: Modify D3.js code in `<script>` section

## Future Enhancements

- Live data updates via WebSocket
- Export reports as PDF
- Interactive source code viewer
- Comparison between multiple runs
- Historical trend analysis
