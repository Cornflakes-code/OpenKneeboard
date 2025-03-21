# If you're using this as an example for something else and having trouble:
# imagemagick requires that '(' and ')' are standalone arguments (i.e. distinct
# entries in argv) - "(foo)" is invalid, but "(" "foo" ")" is valid.
#
# Here, I use `--%` to disable powershell's usual argument parsing for the rest
# of the command
magick convert `
  -background none `
  "$(Get-Location)\OpenKneeboard_Logos_Icon_Color.svg" `
  -gravity center `
  -trim `
  --% ( -clone 0 -resize 16x16 -extent 16x16 ) ( -clone 0 -resize 32x32 -extent 32x32 ) ( -clone 0 -resize 48x48 -extent 48x48 ) ( -clone 0 -resize 64x64 -extent 64x64 ) ( -clone 0 -resize 128x128 -extent 128x128 ) ( -clone 0 -resize 256x256 -extent 256x256 ) icon.ico

# MSI dialog image with left gutter

$dialogWidth=493
$dialogHeight=312
$gutterWidth=164
$margin=16
$overlayWidth=($gutterWidth - (2 * $margin))

magick `
  -background none `
  -size ${dialogWidth}x${dialogHeight} canvas:white `
  -fill '#eee' `
  -draw "rectangle 0,0, ${gutterWidth},${dialogHeight}" `
  '(' `
  "OpenKneeboard_Logos_Flat.svg" `
  -gravity Center `
  -trim -resize $overlayWidth `
  ')' `
  -gravity NorthWest `
  -geometry +$margin+$margin `
  -compose over -composite `
  WiXUIDialog.png

# MSI banner image

$bannerWidth=$dialogWidth
$bannerHeight=58
$iconSize=40
$margin=(($bannerHeight - $iconSize) / 2)

magick `
  -background none `
  -size ${bannerWidth}x${bannerHeight} canvas:white `
  '(' `
  "OpenKneeboard_Logos_Icon_Flat.svg" `
  -gravity Center `
  -trim -resize x$iconSize `
  ')' `
  -gravity NorthEast `
  -geometry +${margin}+$margin `
  -compose over -composite `
  WiXUIBanner.png
