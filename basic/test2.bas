10 DATA 43, 1
11 DATA 46, 1
12 DATA 48, 4
13 DATA 0, 4
14 DATA 43, 1
15 DATA 46, 1
16 DATA 50, 1
17 DATA 48, 4
18 DATA 0, 4
19 DATA 43, 2
20 DATA 46, 2
21 DATA 48, 2
22 DATA 0, 2
23 DATA 46, 4
24 DATA 43, 4
25 DATA 0, 8
100 NOP : rem reset sound
110 FOR X = 1 TO 16 STEP 1
120 READ N, D
130 PLAY N, D
160 NEXT X
165 PRINT "Wait for end"
170 WAIT 53296, 1 :rem hihi
180 RESTORE
190 GOTO  110
