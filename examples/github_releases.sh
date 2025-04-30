#!/bin/bash
# Example: Download GitHub release assets in parallel using Curly
# This script downloads all assets from a GitHub repository's releases

set -e  # Exit on error

# Configuration
REPO=${1:-"nodejs/node"}  # Repository in format "owner/repo"
OUTPUT_DIR=${2:-"./downloads"}
THREADS=${3:-8}
LIMIT=${4:-10}  # Limit number of releases to process

usage() {
    echo "Usage: $0 [REPO] [OUTPUT_DIR] [THREADS] [LIMIT]"
    echo ""
    echo "Example: $0 nodejs/node ./node-releases 16 5"
    echo ""
    echo "Parameters:"
    echo "  REPO       - GitHub repository (format: owner/repo, default: nodejs/node)"
    echo "  OUTPUT_DIR - Directory to save files (default: ./downloads)"
    echo "  THREADS    - Number of download threads (default: 8)"
    echo "  LIMIT      - Maximum number of releases to process (default: 10)"
    exit 1
}

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Temporary files
RELEASES_JSON=$(mktemp)
ASSETS_TSV=$(mktemp)
trap 'rm -f $RELEASES_JSON $ASSETS_TSV' EXIT

echo "Fetching release information for $REPO..."

# Get releases information using GitHub API
curl -s "https://api.github.com/repos/$REPO/releases?per_page=$LIMIT" > "$RELEASES_JSON"

# Check if the API response is valid
if ! grep -q "tag_name" "$RELEASES_JSON"; then
    echo "Error: Failed to fetch releases from GitHub API"
    echo "Check if the repository exists and is public"
    exit 1
fi

# Process each release and extract assets for download
echo "Preparing download list..."
release_count=0
asset_count=0

# Create a TSV file with download tasks
jq -r '.[] | "Release: \(.tag_name) (\(.name))"' "$RELEASES_JSON"
jq -r '.[] | .assets[] | "\(.browser_download_url)\t'"$OUTPUT_DIR"'/\(.name)"' "$RELEASES_JSON" > "$ASSETS_TSV"

# Count releases and assets
release_count=$(jq '. | length' "$RELEASES_JSON")
asset_count=$(wc -l < "$ASSETS_TSV")

echo ""
echo "Found $release_count releases with $asset_count assets"
echo "Starting parallel download with $THREADS threads..."
echo ""

# Download all assets in parallel
if [ "$asset_count" -gt 0 ]; then
    time ../bin/curly_parallel -i "$ASSETS_TSV" -t "$THREADS"
    
    echo ""
    echo "Download complete!"
    
    # Verify downloaded files
    downloaded=$(find "$OUTPUT_DIR" -type f | wc -l)
    echo "Downloaded $downloaded of $asset_count files to $OUTPUT_DIR"
else
    echo "No assets found to download."
fi