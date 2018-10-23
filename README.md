# quake_parser
## Alerts about recent earthquakes based on a contactbook
This C program downloads a text file with the list of the last earthquakes from http://cnt.rm.ingv.it/ using their APIs, and sends them using a Telegram bot to contacts listed in a json file, checking if the contacts only wants news on higher magnitudes.
You should compile it with -DTOKEN=\"1234:abcdefg\" to set the bot's token
Work in Progress
