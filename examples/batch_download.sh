#!/bin/bash
# Example script for batch downloading using Curly's parallel downloader
# Usage: ./batch_download.sh [url_prefix] [file_list] [output_dir] [threads]

set -e  # Exit on error

# Default values
URL_PREFIX="${1:-https://example.com/files/}"
FILE_LIST="${2:-files.txt}"
OUTPUT_DIR="${3:-./downloads}"
THREADS="${4:-8}"

# Check if file list exists
if [ ! -f "$FILE_LIST" ]; then
    echo "Error: File list not found: $FILE_LIST"
    echo "Create a text file with one filename per line"
    exit 1
fi

# Create temporary TSV file
TMP_TSV=$(mktemp)
trap 'rm -f $TMP_TSV' EXIT

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

echo "Preparing download list..."
# Generate TSV file with URL and destination path
while read -r filename; do
    # Skip empty lines and comments
    if [[ -z "$filename" || "$filename" == \#* ]]; then
        continue
    fi
    
    # Construct the full URL and destination path
    url="${URL_PREFIX}${filename}"
    dest="${OUTPUT_DIR}/${filename}"
    
    # Add to TSV file
    echo -e "${url}\t${dest}" >> "$TMP_TSV"
done < "$FILE_LIST"

# Count number of files to download
FILE_COUNT=$(wc -l < "$TMP_TSV")
echo "Ready to download $FILE_COUNT files using $THREADS threads"
echo "Files will be saved to $OUTPUT_DIR"
echo ""

# Start the parallel download
echo "Starting parallel download..."
time ../bin/curly_parallel -i "$TMP_TSV" -t "$THREADS"

echo ""
echo "Download complete!"
echo "Downloaded files are in: $OUTPUT_DIR"

# Optional: Verify downloaded files
DOWNLOADED=$(find "$OUTPUT_DIR" -type f | wc -l)
echo "Successfully downloaded: $DOWNLOADED / $FILE_COUNT files"