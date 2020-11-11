# Trans2Png
## What does Transparent To Png do?
For images with transparency, it change the source image to png, setting the transparent pixel to background color.

It is used in conjunction with SyncUpsImage. Run Trans2Png to convert transparent background to background color before passing to ESRGanResizer.  
This is due to ESRGan unable to process transparent pixel properly. Then run SyncUpsImage to change the transparent pixels in source to transparent in destination.

## Where do I get the deps library and executable file?
You can get it from the link below  
[Library & binary - https://drive.google.com/drive/folders/1lvLiKO_8OEO04AVEqK4_LsEDY5xIFs3f?usp=sharing](https://drive.google.com/drive/folders/1lvLiKO_8OEO04AVEqK4_LsEDY5xIFs3f?usp=sharing)

## How to use it?
Run the below command  
  Trans2Png "C:\dest"

Note that it will replace the images in destination with the new png with background color.
