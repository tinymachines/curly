#!/bin/bash
# Advanced example: Dynamic batch downloading with Curly
# This script demonstrates how to dynamically generate download tasks
# and download them in parallel batches

set -e  # Exit on error

# Configuration
API_URL="https://httpbin.org"
OUTPUT_DIR="./downloaded_data"
THREADS=4
BATCH_SIZE=100
TOTAL_ITEMS=500  # For demonstration, limit to 500 items

usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -o, --output DIR    Set output directory (default: $OUTPUT_DIR)"
    echo "  -t, --threads NUM   Set number of download threads (default: $THREADS)"
    echo "  -b, --batch NUM     Set batch size (default: $BATCH_SIZE)"
    echo "  -n, --total NUM     Set total number of items (default: $TOTAL_ITEMS)"
    echo "  -h, --help          Show this help message"
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -t|--threads)
            THREADS="$2"
            shift 2
            ;;
        -b|--batch)
            BATCH_SIZE="$2"
            shift 2
            ;;
        -n|--total)
            TOTAL_ITEMS="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Create output directories
mkdir -p "$OUTPUT_DIR"/{images,json,data}

# Temporary TSV file for current batch
TMP_TSV=$(mktemp)
trap 'rm -f $TMP_TSV' EXIT

# Function to generate a sample item download task
# Args: $1 - item number
generate_item() {
    local item_num=$1
    local mod=$(( item_num % 3 ))
    
    case $mod in
        0)  # Image files
            local format=$(( RANDOM % 3 ))
            case $format in
                0) echo -e "${API_URL}/image/jpeg\t${OUTPUT_DIR}/images/image_${item_num}.jpg" ;;
                1) echo -e "${API_URL}/image/png\t${OUTPUT_DIR}/images/image_${item_num}.png" ;;
                2) echo -e "${API_URL}/image/svg\t${OUTPUT_DIR}/images/image_${item_num}.svg" ;;
            esac
            ;;
        1)  # JSON data
            echo -e "${API_URL}/json\t${OUTPUT_DIR}/json/data_${item_num}.json"
            ;;
        2)  # Other data types
            local format=$(( RANDOM % 3 ))
            case $format in
                0) echo -e "${API_URL}/xml\t${OUTPUT_DIR}/data/data_${item_num}.xml" ;;
                1) echo -e "${API_URL}/html\t${OUTPUT_DIR}/data/page_${item_num}.html" ;;
                2) echo -e "${API_URL}/robots.txt\t${OUTPUT_DIR}/data/robot_${item_num}.txt" ;;
            esac
            ;;
    esac
}

echo "Starting dynamic download process"
echo "Total items: $TOTAL_ITEMS"
echo "Batch size: $BATCH_SIZE"
echo "Threads: $THREADS"
echo "Output directory: $OUTPUT_DIR"
echo ""

# Process in batches
total_downloaded=0
batch_num=1

while [ $total_downloaded -lt $TOTAL_ITEMS ]; do
    # Calculate items in this batch
    items_left=$(( TOTAL_ITEMS - total_downloaded ))
    batch_items=$(( items_left > BATCH_SIZE ? BATCH_SIZE : items_left ))
    
    echo "Processing batch #$batch_num ($batch_items items)"
    
    # Clear the TSV file
    > "$TMP_TSV"
    
    # Generate batch download tasks
    for (( i=1; i<=batch_items; i++ )); do
        item_num=$(( total_downloaded + i ))
        generate_item $item_num >> "$TMP_TSV"
    done
    
    # Execute parallel download for this batch
    echo "Downloading batch #$batch_num..."
    ../bin/curly_parallel -i "$TMP_TSV" -t "$THREADS"
    
    # Update counters
    total_downloaded=$(( total_downloaded + batch_items ))
    batch_num=$(( batch_num + 1 ))
    
    echo "Completed $total_downloaded of $TOTAL_ITEMS items"
    echo ""
    
    # Optional: Add a small delay between batches (for API rate limiting)
    if [ $total_downloaded -lt $TOTAL_ITEMS ]; then
        echo "Waiting 2 seconds before next batch..."
        sleep 2
    fi
done

echo "Download process complete!"
echo "Downloaded $total_downloaded items to $OUTPUT_DIR"

# Generate a summary
image_count=$(find "$OUTPUT_DIR/images" -type f | wc -l)
json_count=$(find "$OUTPUT_DIR/json" -type f | wc -l)
data_count=$(find "$OUTPUT_DIR/data" -type f | wc -l)

echo ""
echo "Download summary:"
echo "- Images: $image_count files"
echo "- JSON: $json_count files"
echo "- Other data: $data_count files"
echo "- Total: $(( image_count + json_count + data_count )) files"