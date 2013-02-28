
;; test comment

;; test string
"test string\n \a \f"

;; test char
?\n

;; test decimal integer
100

;; test negitive decimal integer
-100

;; test hexdecimal integer
0xff

;; test symbol
dump

;; test simple list
(dump "test\n")

;; test long list
(testing 1 2 3 4 5 6 7 8 9 10 testing)

;; test simple pair
(dump . "test\n")

;; test quote list
'(one anda two anda three)

;; test quasiquote list
`(this is a fine day)

;; test unquote list
,(testing one two three)

;; test unquote_splicing list
,@(testing one two three)

;; test [ list
[testing 1 2 3 4 5 6 7 8 9 10 . testing]

;; test delay list
{testing 1 2 3 4 5 6 7 8 9 10 . testing}

;; test path 
one.two.three

;; test
_
