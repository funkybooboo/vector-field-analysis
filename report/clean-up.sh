#!/bin/bash

echo "Cleaning up LaTeX auxiliary files in current directory and subdirectories..."

# List of extensions to clean
EXTENSIONS="aux log out toc lof lot fls fdb_latexmk synctex.gz bbl blg idx ind ilg nav snm vrb thm xdy run.xml bcf ist glg gls glo glsdefs xdv bak*"

# Counter for total files removed
total_removed=0

for ext in $EXTENSIONS; do
	# Find files with the extension and count them
	files=$(find . -type f -name "*.$ext" 2>/dev/null)
	count=$(echo "$files" | grep -c ".*" || echo 0)

	if [ "$count" -gt 0 ]; then
		echo "Removing $count *.$ext files..."
		find . -type f -name "*.$ext" -delete
		total_removed=$((total_removed + count))
	fi
done

echo "Cleanup complete. Removed $total_removed files total."
