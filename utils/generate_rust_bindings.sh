#!/bin/bash

script_dir=$(dirname $0)

output_dir=$1
if [ -z "$output_dir" ]; then
	echo "Missing output directory path."
	echo "Usage: $0 <output_path> [<header_file_path>...]"
	exit 1
fi

shift
header_paths=("$@")


mkdir -p $output_dir

for header in ${header_paths[@]}; do
	header_name=$(basename $header .h)

	echo "Generating ($header) -> ($output_dir/$header_name.rs)"
	bindgen "$header" --match="$header" --convert-macros --use-core -- -I${script_dir}/../build/install/include -I/usr/arm-none-eabi/include -target arm-none-eabi > "$output_dir/$header_name.rs"
done
