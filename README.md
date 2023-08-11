# timetracking

Track the time you spend doing specified activities.

## usage

Create a file that lists your general time sinks. It is recommended not to use too many.  
It may look like this:
```
# this is a comment
# names of activities (first column), may be 4 characters long
- sleeping
C coding
Eat eating
W # activities dont have to have descriptions, and can always have comments
M music
```
This file **must** include a trailing empty new line.

Create a second, empty, file. This will contain the log of your activities over time.
If you wish to edit it by hand, it may look like this:
```
# comments allowed in log too, automatically pushed entries will look like this
2023-08-11_10.10 W
2023-08-11_12.30 W M # can also have multiple activities at once, use sparingly
2023-08-11_20.00 -

# when writing entries yourself, you can use the much shorter format
2023-08-12 # introduces a new day
07.00 Eat
08.00 -
09.00 C
23.50 Eat
# you must then introduce a new day to go forward
2023-08-13
00.30 -
```
This file **must** include a trailing empty new line.

Otherwise you may also push new entries into this file by using the push mode of the timetracking tool. This will automatically check if the sinks you attempt to push actually exist.  
```$ timetracking <path to sinks file> <path to log file> push <one or more activities>```  
It is recommended to setup a custom script to use this, so you can both get an easier name and not have to input the file paths everytime.
