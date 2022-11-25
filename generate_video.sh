#!/usr/bin/bash

# set -Eeuf -o pipefail && set -x

get_tts () {
  name=$( echo "$1" | head -c 30 )

  if [ -f "cache/$name".wav ]; then
    echo "--> Cache hit"
    cp "cache/$name".wav tmp/$2.wav
  else 
    echo "Cache miss"

    curl --request POST 'http://localhost:8899/synthesize/' \
       --header 'Content-Type: application/json' \
       --data-raw "{
       \"text\": \"$1\",
       \"voice\": \"Ruslan\"
      }" | jq .response[0].response_audio | sed 's/"//g' | base64 --decode > "cache/$name".wav

    cp "cache/$name".wav tmp/$2.wav
  fi
}

create_video_chunk () {
  yes | ffmpeg -i 2SecSilence.wav -i tmp/$1.wav -i 1SecSilence.wav -filter_complex "[0:a][1:a][2:a]concat=n=3:v=0:a=1" tmp/$1.with_pause.wav
  yes | ffmpeg -loop 1 -i tmp/$1.jpg -i tmp/$1.with_pause.wav -shortest -acodec copy -vcodec h264 tmp/$1.mkv
  echo "file '$i.mkv'" >> tmp/videos.txt
}

# rm -rf render/tmp
mkdir -p render/tmp
cd render

echo ":: Converting pdf to jpg"

# convert -density 200 main.pdf -quality 90 tmp/%d.jpg

input="voice.txt"

i=0

echo ":: TTS"

while IFS= read -r line
do
  get_tts "$line" $i
  
  create_video_chunk $i

  i=$((i+1))
done < "$input"

echo ":: Converting to lecture"

yes | ffmpeg -f concat -i tmp/videos.txt -c:a copy -c:v copy lecture.mkv
