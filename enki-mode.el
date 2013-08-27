;;; enki-mode.el --- Enki mode, and its idiosyncratic commands

(defconst enki-font-lock-keywords-1
    (list
     ;;
     ;; Definitions.
     (list 
      (concat
       "(" (regexp-opt
            '(
              "define"
              "lambda" "pi" "sigma" "any" "all" "subset"
              "delay" "syntax" "ctor"
              "bind"
              "set"
              "macro"
              ) t)
       "\\>")
      '(1 font-lock-keyword-face))
     )
  "Subdued level highlighting for Enki modes.")

(defconst enki-font-lock-keywords-2
  (append enki-font-lock-keywords-1
     (list
      ;;
      ;; Control structures.
      (cons (concat
	     "(" (regexp-opt
		  '(
                    "and"
                    "begin"
                    "cond"
                    "if"
                    "let"
                    "fix"
                    "or"
                    "case"
                    "effect"
                    "_"
                    ) t)
	     "\\>")
	    1)
      ;;
      ;; Control structures
      (cons (concat
	     "(" (regexp-opt
		  '(
		    "loop"
                    "do"
                    "require"
                    "unless"
                    "when"
                    ) t)
	     "\\>")
	    1)
      ;;
      ;; Exit/Feature symbols as constants.
      (list
       (concat
        "(" (regexp-opt
             '(
               "catch"
               "featurep"
               "provide"
               "throw"
               ) t)
        "\\>"
        "[ \t']*\\(\\sw+\\)?")
       '(1 font-lock-keyword-face)
       '(2 font-lock-constant-face nil t))
      ;;
      ;; Erroneous structures.
      (list
       (concat
        "(" (regexp-opt
             '(
               "abort" 
               "assert" 
               "error"
               "signal"
               ) t)
        "\\>")
       1 font-lock-warning-face)
      ;;
      ;;
      ;; Words inside \\s' tend to be symbol names.
      '("\\s'\\(\\sw+\\)" 1 font-lock-constant-face append)
      ;;
      ;;
      '("\\.\\(\\sw+\\)" 1 font-lock-function-name-face append)

      ;;
      ;; Constant values.
      '(":\\(\\sw+\\)" 1 font-lock-builtin-face append)
      ;;
      ;; Enki `&' keywords as types.
      '("&\\(\\sw+\\)"  1 font-lock-type-face append)
      ))
  "Gaudy level highlighting for Enki modes.")

(defvar enki-font-lock-keywords enki-font-lock-keywords-1
  "Default expressions to highlight in Enki modes.")

(defvar enki-mode-abbrev-table nil)

(defvar enki-mode-syntax-table
  (let ((table (make-syntax-table)))
    (let ((i 0))
      (while (< i ?0)
	(modify-syntax-entry i "_   " table)
	(setq i (1+ i)))
      (setq i (1+ ?9))
      (while (< i ?A)
	(modify-syntax-entry i "_   " table)
	(setq i (1+ i)))
      (setq i (1+ ?Z))
      (while (< i ?a)
	(modify-syntax-entry i "_   " table)
	(setq i (1+ i)))
      (setq i (1+ ?z))
      (while (< i 128)
	(modify-syntax-entry i "_   " table)
	(setq i (1+ i)))

      ;; symbols
      (modify-syntax-entry ?$ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?% "_   " table) ;; symbol constituent
      (modify-syntax-entry ?& "_   " table) ;; symbol constituent
      (modify-syntax-entry ?* "_   " table) ;; symbol constituent
      (modify-syntax-entry ?+ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?- "_   " table) ;; symbol constituent
      (modify-syntax-entry ?/ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?< "_   " table) ;; symbol constituent
      (modify-syntax-entry ?= "_   " table) ;; symbol constituent
      (modify-syntax-entry ?> "_   " table) ;; symbol constituent
      (modify-syntax-entry ?^ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?| "_   " table) ;; symbol constituent
      (modify-syntax-entry ?~ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?! "_   " table) ;; symbol constituent

      ;; whitespace
      (modify-syntax-entry ?   "     " table) ;; whitespace
      (modify-syntax-entry ?\t "     " table) ;; whitespace
      (modify-syntax-entry ?\f "     " table) ;; whitespace

      ;; comment
      (modify-syntax-entry ?\#  "< 124" table) ;; whitespace as well as for ## comments
      (modify-syntax-entry ?\n  ">    " table)
      (modify-syntax-entry ?\^m ">    " table)

      ;; word
      (modify-syntax-entry ?_  "w   " table) ;; word constituent

      ;; expression prefix
      (modify-syntax-entry ?`  "'   " table) ;; expression prefix
      (modify-syntax-entry ?'  "'   " table) ;; expression prefix
      (modify-syntax-entry ?@  "'   " table) ;; expression prefix

      ;; punctuation
      (modify-syntax-entry ?,  ".   " table) ;; punctuation character
      (modify-syntax-entry ?\; ".   " table) ;; punctuation character
      (modify-syntax-entry ?:  ".   " table) ;; punctuation character
      (modify-syntax-entry ?.  ".   " table) ;; punctuation character

      ;; quoting
      (modify-syntax-entry ?\" "\"  " table) ;; string quote
      (modify-syntax-entry ?\\ "\\  " table) ;; escape character

      ;; expression delimiters
      (modify-syntax-entry ?\( "() 2b" table) ;; open  (
      (modify-syntax-entry ?\) ")( 3b" table) ;; close )
      (modify-syntax-entry ?\[ "(] 2b" table) ;; open  [
      (modify-syntax-entry ?\] ")[ 3b" table) ;; close ]
      (modify-syntax-entry ?\{ "(} 2b" table) ;; open  {
      (modify-syntax-entry ?\} "){ 3b" table) ;; close }
      )
    table))

(defvar emacs-enki-mode-syntax-table (copy-syntax-table enki-mode-syntax-table))

(define-abbrev-table 'enki-mode-abbrev-table ())

(defun enki-mode-variables ()
  (make-local-variable 'paragraph-start)
  (make-local-variable 'paragraph-separate)
  (make-local-variable 'paragraph-ignore-fill-prefix)
  (make-local-variable 'fill-paragraph-function)
  (make-local-variable 'adaptive-fill-mode)
  (make-local-variable 'normal-auto-fill-function)
  (make-local-variable 'indent-line-function)
  (make-local-variable 'indent-region-function)
  (make-local-variable 'parse-sexp-ignore-comments)
  (make-local-variable 'outline-regexp)
  (make-local-variable 'outline-level)
  (make-local-variable 'comment-start)
  (make-local-variable 'comment-start-skip)
  (make-local-variable 'comment-add)
  (make-local-variable 'comment-column)
  (make-local-variable 'comment-indent-function)
  (make-local-variable 'multibyte-syntax-as-symbol)

  (set-syntax-table enki-mode-syntax-table)
  (setq local-abbrev-table enki-mode-abbrev-table)
  (setq paragraph-start (concat page-delimiter "\\|$" ))  
  (setq paragraph-separate paragraph-start)
  (setq paragraph-ignore-fill-prefix t)
  (setq fill-paragraph-function 'enki-fill-paragraph)

  ;; Adaptive fill mode gets in the way of auto-fill,
  ;; and should make no difference for explicit fill
  ;; because enki-fill-paragraph should do the job.

  (setq adaptive-fill-mode nil)
  (setq normal-auto-fill-function 'enki-mode-auto-fill)
  (setq indent-line-function 'enki-indent-line)
  (setq indent-region-function 'enki-indent-region)
  (setq parse-sexp-ignore-comments t)
  (setq outline-regexp "#[ \\|(")
  (setq outline-level 'enki-outline-level)
  (setq comment-start "#")

  ;; Look within the line for a # following an even number of backslashes
  ;; after either a non-backslash or the line beginning.
  (setq comment-start-skip "\\(\\(^\\|[^\\\\\n]\\)\\(\\\\\\\\\\)*\\)\\s<+ *")
  (setq comment-add 1)
  (setq comment-column 40)
  (setq comment-indent-function 'enki-comment-indent)
  (setq multibyte-syntax-as-symbol t)
  (setq font-lock-defaults
	'((enki-font-lock-keywords
	   enki-font-lock-keywords-1
           enki-font-lock-keywords-2)
	  nil
          nil
          (("+-*/.<>=!?$%_&~^:" . "w"))
          beginning-of-defun
	  (font-lock-mark-block-function . mark-defun))))

(defun enki-outline-level ()
  "Enki mode `outline-level' function."
  (if (looking-at "(")
      1000
    (looking-at outline-regexp)
    (- (match-end 0) (match-beginning 0))))

(defvar enki-mode-shared-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\t"     'enki-indent-line)
    (define-key map "\e\C-q" 'enki-indent-sexp)
    (define-key map "\177"   'backward-delete-char-untabify)
    ;; This gets in the way when viewing a Enki file in view-mode.  As
    ;; long as [backspace] is mapped into DEL via the
    ;; function-key-map, this should remain disabled!!
    ;;;(define-key map [backspace] 'backward-delete-char-untabify)
    map)
  "Keymap for commands shared by all sorts of Enki modes.")

(defcustom enki-mode-hook nil
  "Hook run when entering Enki mode."
  :options '(imenu-add-menubar-index)
  :type 'hook
  :group 'enki)

(defvar enki-mode-map
  (let ((map (make-sparse-keymap)))
    (set-keymap-parent map enki-mode-shared-map)
    map)
  "Keymap for ordinary Enki mode.
All commands in `enki-mode-shared-map' are inherited by this map.")

(define-derived-mode enki-mode nil "Enki"
  "Major mode for editing Enki code.
Commands:
Delete converts tabs to spaces as it moves back.
Blank lines separate paragraphs.  Semicolons start comments.
\\{enki-mode-map}

Entry to this mode calls the value of `enki-mode-hook'
if that value is non-nil."
  (enki-mode-variables)
  (make-local-variable 'font-lock-keywords-case-fold-search)
  (setq font-lock-keywords-case-fold-search t)
  (setq imenu-case-fold-search t))

(defun enki-comment-indent ()
  (if (or (looking-at "\\s<!")         ;;; #!/xxxxx
          (looking-at "\\s<\\s(")      ;;; #[...
          (looking-at "\\s<\\s<\\s<")) ;;; ###         
      (current-column)
    (if (looking-at "\\s<\\s<")
	(let ((tem (or (calculate-enki-indent) (current-column))))
	  (if (listp tem) (car tem) tem))
      (skip-chars-backward " \t")
      (max (if (bolp) 0 (1+ (current-column)))
	   comment-column))))

(defun enki-mode-auto-fill ()
  (if (> (current-column) (current-fill-column))
      (if (save-excursion
	    (nth 4 (parse-partial-sexp (save-excursion
					 (beginning-of-defun)
					 (point))
				       (point))))
	  (do-auto-fill)
	(let ((comment-start nil) (comment-start-skip nil))
	  (do-auto-fill)))))

(defvar enki-indent-offset nil
  "If non-nil, indent second line of expressions that many more columns.")
(defvar enki-indent-function 'enki-indent-function)

(defun enki-indent-line (&optional whole-exp)
  "Indent current line as Enki code.
With argument, indent any additional lines of the same expression
rigidly along with this one."
  (interactive "P")
  (let ((indent (calculate-enki-indent)) shift-amt beg end
	(pos (- (point-max) (point))))
    (beginning-of-line)
    (setq beg (point))
    (skip-chars-forward " \t")
    (if (or (null indent)
            (looking-at "\\s<!")
            (looking-at "\\s<\\s(")
            (looking-at "\\s<\\s<\\s<"))
	;; Don't alter indentation of a #[ or #! or ### comment line
	;; or a line that starts in a string.
	(goto-char (- (point-max) pos))
      (if (and (looking-at "\\s<")
               (not (looking-at "\\s<!"))
               (not (looking-at "\\s<\\s("))
               (not (looking-at "\\s<\\s<")))
	  ;; Single-hash comment lines should be indented
	  ;; as comment lines, not as code.
	  (progn (indent-for-comment) (forward-char -1))
	(if (listp indent) (setq indent (car indent)))
	(setq shift-amt (- indent (current-column)))
	(if (zerop shift-amt)
	    nil
	  (delete-region beg (point))
	  (indent-to indent)))
      ;; If initial point was within line's indentation,
      ;; position after the indentation.  Else stay at same point in text.
      (if (> (- (point-max) pos) (point))
	  (goto-char (- (point-max) pos)))
      ;; If desired, shift remaining lines of expression the same amount.
      (and whole-exp (not (zerop shift-amt))
	   (save-excursion
	     (goto-char beg)
	     (forward-sexp 1)
	     (setq end (point))
	     (goto-char beg)
	     (forward-line 1)
	     (setq beg (point))
	     (> end beg))
	   (enki-indent-code-rigidly beg end shift-amt)))))

(defvar calculate-enki-indent-last-sexp)

(defun calculate-enki-indent (&optional parse-start)
  "Return appropriate indentation for current line as Enki code.
In usual case returns an integer: the column to indent to.
If the value is nil, that means don't change the indentation
because the line starts inside a string.

The value can also be a list of the form (COLUMN CONTAINING-SEXP-START).
This means that following lines at the same level of indentation
should not necessarily be indented the same as this line.
Then COLUMN is the column to indent to, and CONTAINING-SEXP-START
is the buffer position of the start of the containing expression."
  (save-excursion
    (beginning-of-line)
    (let ((indent-point (point))
          state paren-depth
          ;; setting this to a number inhibits calling hook
          (desired-indent nil)
          (retry t)
          calculate-enki-indent-last-sexp containing-sexp)
      (if parse-start
          (goto-char parse-start)
          (beginning-of-defun))
      ;; Find outermost containing sexp
      (while (< (point) indent-point)
        (setq state (parse-partial-sexp (point) indent-point 0)))
      ;; Find innermost containing sexp
      (while (and retry
		  state
                  (> (setq paren-depth (elt state 0)) 0))
        (setq retry nil)
        (setq calculate-enki-indent-last-sexp (elt state 2))
        (setq containing-sexp (elt state 1))
        ;; Position following last unclosed open.
        (goto-char (1+ containing-sexp))
        ;; Is there a complete sexp since then?
        (if (and calculate-enki-indent-last-sexp
		 (> calculate-enki-indent-last-sexp (point)))
            ;; Yes, but is there a containing sexp after that?
            (let ((peek (parse-partial-sexp calculate-enki-indent-last-sexp
					    indent-point 0)))
              (if (setq retry (car (cdr peek))) (setq state peek)))))
      (if retry
          nil
        ;; Innermost containing sexp found
        (goto-char (1+ containing-sexp))
        (if (not calculate-enki-indent-last-sexp)
	    ;; indent-point immediately follows open paren.
	    ;; Don't call hook.
            (setq desired-indent (current-column))
	  ;; Find the start of first element of containing sexp.
	  (parse-partial-sexp (point) calculate-enki-indent-last-sexp 0 t)
	  (cond ((looking-at "\\s(")
		 ;; First element of containing sexp is a list.
		 ;; Indent under that list.
		 )
		((> (save-excursion (forward-line 1) (point))
		    calculate-enki-indent-last-sexp)
		 ;; This is the first line to start within the containing sexp.
		 ;; It's almost certainly a function call.
		 (if (= (point) calculate-enki-indent-last-sexp)
		     ;; Containing sexp has nothing before this line
		     ;; except the first element.  Indent under that element.
		     nil
		   ;; Skip the first element, find start of second (the first
		   ;; argument of the function call) and indent under.
		   (progn (forward-sexp 1)
			  (parse-partial-sexp (point)
					      calculate-enki-indent-last-sexp
					      0 t)))
		 (backward-prefix-chars))
		(t
		 ;; Indent beneath first sexp on same line as
		 ;; `calculate-enki-indent-last-sexp'.  Again, it's
		 ;; almost certainly a function call.
		 (goto-char calculate-enki-indent-last-sexp)
		 (beginning-of-line)
		 (parse-partial-sexp (point) calculate-enki-indent-last-sexp
				     0 t)
		 (backward-prefix-chars)))))
      ;; Point is at the point to indent under unless we are inside a string.
      ;; Call indentation hook except when overridden by enki-indent-offset
      ;; or if the desired indentation has already been computed.
      (let ((normal-indent (current-column)))
        (cond ((elt state 3)
               ;; Inside a string, don't change indentation.
	       nil)
              ((and (integerp enki-indent-offset) containing-sexp)
               ;; Indent by constant offset
               (goto-char containing-sexp)
               (+ (current-column) enki-indent-offset))
              (desired-indent)
              ((and (boundp 'enki-indent-function)
                    enki-indent-function
                    (not retry))
               (or (funcall enki-indent-function indent-point state)
                   normal-indent))
              (t
               normal-indent))))))

(defun enki-indent-function (indent-point state)
  (let ((normal-indent (current-column)))
    (goto-char (1+ (elt state 1)))
    (parse-partial-sexp (point) calculate-enki-indent-last-sexp 0 t)
    (if (and (elt state 2)
             (not (looking-at "\\sw\\|\\s_")))
        ;; car of form doesn't seem to be a symbol
        (progn
          (if (not (> (save-excursion (forward-line 1) (point))
                      calculate-enki-indent-last-sexp))
              (progn (goto-char calculate-enki-indent-last-sexp)
                     (beginning-of-line)
                     (parse-partial-sexp (point)
					 calculate-enki-indent-last-sexp 0 t)))
          ;; Indent under the list or under the first sexp on the same
          ;; line as calculate-enki-indent-last-sexp.  Note that first
          ;; thing on that line has to be complete sexp since we are
          ;; inside the innermost containing sexp.
          (backward-prefix-chars)
          (current-column))
      (let ((function (buffer-substring (point)
					(progn (forward-sexp 1) (point))))
	    method)
	(setq method (or (get (intern-soft function) 'enki-indent-function)
			 (get (intern-soft function) 'enki-indent-hook)))
	(cond ((or (eq method 'define)
		   (and (null method)
			(> (length function) 6)
			(string-match "\\`define" function)))
	       (enki-indent-defform state indent-point))
	      ((integerp method)
	       (enki-indent-specform method state
				     indent-point normal-indent))
	      (method
		(funcall method state indent-point)))))))

(defvar enki-body-indent 3
  "Number of columns to indent the second line of a `(def...)' form.")

(defun enki-indent-specform (count state indent-point normal-indent)
  (let ((containing-form-start (elt state 1))
        (i count)
        body-indent containing-form-column)
    ;; Move to the start of containing form, calculate indentation
    ;; to use for non-distinguished forms (> count), and move past the
    ;; function symbol.  enki-indent-function guarantees that there is at
    ;; least one word or symbol character following open paren of containing
    ;; form.
    (goto-char containing-form-start)
    (setq containing-form-column (current-column))
    (setq body-indent (+ enki-body-indent containing-form-column))
    (forward-char 1)
    (forward-sexp 1)
    ;; Now find the start of the last form.
    (parse-partial-sexp (point) indent-point 1 t)
    (while (and (< (point) indent-point)
                (condition-case ()
                    (progn
                      (setq count (1- count))
                      (forward-sexp 1)
                      (parse-partial-sexp (point) indent-point 1 t))
                  (error nil))))
    ;; Point is sitting on first character of last (or count) sexp.
    (if (> count 0)
        ;; A distinguished form.  If it is the first or second form use double
        ;; enki-body-indent, else normal indent.  With enki-body-indent bound
        ;; to 2 (the default), this just happens to work the same with if as
        ;; the older code, but it makes unwind-protect, condition-case,
        ;; with-output-to-temp-buffer, et. al. much more tasteful.  The older,
        ;; less hacked, behavior can be obtained by replacing below with
        ;; (list normal-indent containing-form-start).
        (if (<= (- i count) 1)
            (list (+ containing-form-column (* 2 enki-body-indent))
                  containing-form-start)
            (list normal-indent containing-form-start))
      ;; A non-distinguished form.  Use body-indent if there are no
      ;; distinguished forms and this is the first undistinguished form,
      ;; or if this is the first undistinguished form and the preceding
      ;; distinguished form has indentation at least as great as body-indent.
      (if (or (and (= i 0) (= count 0))
              (and (= count 0) (<= body-indent normal-indent)))
          body-indent
          normal-indent))))

(defun enki-indent-defform (state indent-point)
  (goto-char (car (cdr state)))
  (forward-line 1)
  (if (> (point) (car (cdr (cdr state))))
      (progn
	(goto-char (car (cdr state)))
	(+ enki-body-indent (current-column)))))


;; (put 'begin 'enki-indent-function 0), say, causes 'begin' to be indented
;; like define if the first form is placed on the next line, otherwise
;; it is indented like any other form (i.e. forms line up under first).

(put 'lambda 'enki-indent-function 'define)
(put 'macro  'enki-indent-function 'define)
(put 'define 'enki-indent-function 'define)
(put 'begin  'enki-indent-function 0)
(put 'progn  'enki-indent-function 0)
(put 'prog1  'enki-indent-function 1)
(put 'prog2  'enki-indent-function 2)
(put 'let    'enki-indent-function 1)
(put 'let*   'enki-indent-function 1)
(put 'while  'enki-indent-function 1)
(put 'if     'enki-indent-function 2)
(put 'unless 'enki-indent-function 2)
(put 'catch  'enki-indent-function 1)
(put 'when   'enki-indent-function 1)
(put 'unless 'enki-indent-function 1)

(defun enki-indent-sexp (&optional endpos)
  "Indent each line of the list starting just after point.
If optional arg ENDPOS is given, indent each line, stopping when
ENDPOS is encountered."
  (interactive)
  (let ((indent-stack (list nil))
	(next-depth 0)
	;; If ENDPOS is non-nil, use nil as STARTING-POINT
	;; so that calculate-enki-indent will find the beginning of
	;; the defun we are in.
	;; If ENDPOS is nil, it is safe not to scan before point
	;; since every line we indent is more deeply nested than point is.
	(starting-point (if endpos nil (point)))
	(last-point (point))
	last-depth bol outer-loop-done inner-loop-done state this-indent)
    (or endpos
	;; Get error now if we don't have a complete sexp after point.
	(save-excursion (forward-sexp 1)))
    (save-excursion
      (setq outer-loop-done nil)
      (while (if endpos (< (point) endpos)
	       (not outer-loop-done))
	(setq last-depth next-depth
	      inner-loop-done nil)
	;; Parse this line so we can learn the state
	;; to indent the next line.
	;; This inner loop goes through only once
	;; unless a line ends inside a string.
	(while (and (not inner-loop-done)
		    (not (setq outer-loop-done (eobp))))
	  (setq state (parse-partial-sexp (point) (progn (end-of-line) (point))
					  nil nil state))
	  (setq next-depth (car state))
	  ;; If the line contains a comment other than the sort
	  ;; that is indented like code,
	  ;; indent it now with indent-for-comment.
	  ;; Comments indented like code are right already.
	  ;; In any case clear the in-comment flag in the state
	  ;; because parse-partial-sexp never sees the newlines.
	  (if (car (nthcdr 4 state))
	      (progn (indent-for-comment)
		     (end-of-line)
		     (setcar (nthcdr 4 state) nil)))
	  ;; If this line ends inside a string,
	  ;; go straight to next line, remaining within the inner loop,
	  ;; and turn off the \-flag.
	  (if (car (nthcdr 3 state))
	      (progn
		(forward-line 1)
		(setcar (nthcdr 5 state) nil))
	    (setq inner-loop-done t)))
	(and endpos
	     (<= next-depth 0)
	     (progn
	       (setq indent-stack (nconc indent-stack
					 (make-list (- next-depth) nil))
		     last-depth (- last-depth next-depth)
		     next-depth 0)))
	(or outer-loop-done endpos
	    (setq outer-loop-done (<= next-depth 0)))
	(if outer-loop-done
	    (forward-line 1)
	  (while (> last-depth next-depth)
	    (setq indent-stack (cdr indent-stack)
		  last-depth (1- last-depth)))
	  (while (< last-depth next-depth)
	    (setq indent-stack (cons nil indent-stack)
		  last-depth (1+ last-depth)))
	  ;; Now go to the next line and indent it according
	  ;; to what we learned from parsing the previous one.
	  (forward-line 1)
	  (setq bol (point))
	  (skip-chars-forward " \t")
	  ;; But not if the line is blank, or just a comment
	  ;; (except for double-semi comments; indent them as usual).
	  (if (or (eobp) (looking-at "\\s<\\|\n"))
	      nil
	    (if (and (car indent-stack)
		     (>= (car indent-stack) 0))
		(setq this-indent (car indent-stack))
	      (let ((val (calculate-enki-indent
			  (if (car indent-stack) (- (car indent-stack))
			    starting-point))))
		(if (null val)
		    (setq this-indent val)
		  (if (integerp val)
		      (setcar indent-stack
			      (setq this-indent val))
		    (setcar indent-stack (- (car (cdr val))))
		    (setq this-indent (car val))))))
	    (if (and this-indent (/= (current-column) this-indent))
		(progn (delete-region bol (point))
		       (indent-to this-indent)))))
	(or outer-loop-done
	    (setq outer-loop-done (= (point) last-point))
	    (setq last-point (point)))))))

(defun enki-indent-region (start end)
  "Indent every line whose first char is between START and END inclusive."
  (save-excursion
    (let ((endmark (copy-marker end)))
      (goto-char start)
      (and (bolp) (not (eolp))
	   (enki-indent-line))
      (enki-indent-sexp endmark)
      (set-marker endmark nil))))

;;;; Enki paragraph filling commands.

(defun enki-fill-paragraph (&optional justify)
  "Like \\[fill-paragraph], but handle Emacs Enki comments.
If any of the current line is a comment, fill the comment or the
paragraph of it that point is in, preserving the comment's indentation
and initial semicolons."
  (interactive "P")
  (let (
	;; Non-nil if the current line contains a comment.
	has-comment

	;; Non-nil if the current line contains code and a comment.
	has-code-and-comment

	;; If has-comment, the appropriate fill-prefix for the comment.
	comment-fill-prefix
	)

    ;; Figure out what kind of comment we are looking at.
    (save-excursion
      (beginning-of-line)
      (cond

       ;; A line with nothing but a comment on it?
       ((looking-at "[ \t]*;[; \t]*")
	(setq has-comment t
	      comment-fill-prefix (buffer-substring (match-beginning 0)
						    (match-end 0))))

       ;; A line with some code, followed by a comment?  Remember that the
       ;; semi which starts the comment shouldn't be part of a string or
       ;; character.
       ((condition-case nil
	    (save-restriction
	      (narrow-to-region (point-min)
				(save-excursion (end-of-line) (point)))
	      (while (not (looking-at ";\\|$"))
		(skip-chars-forward "^;\n\"\\\\?")
		(cond
		 ((eq (char-after (point)) ?\\) (forward-char 2))
		 ((memq (char-after (point)) '(?\" ??)) (forward-sexp 1))))
	      (looking-at ";+[\t ]*"))
	  (error nil))
	(setq has-comment t has-code-and-comment t)
	(setq comment-fill-prefix
	      (concat (make-string (/ (current-column) 8) ?\t)
		      (make-string (% (current-column) 8) ?\ )
		      (buffer-substring (match-beginning 0) (match-end 0)))))))

    (if (not has-comment)
        ;; `paragraph-start' is set here (not in the buffer-local
        ;; variable so that `forward-paragraph' et al work as
        ;; expected) so that filling (doc) strings works sensibly.
        ;; Adding the opening paren to avoid the following sexp being
        ;; filled means that sexps generally aren't filled as normal
        ;; text, which is probably sensible.  The `;' and `:' stop the
        ;; filled para at following comment lines and keywords
        ;; (typically in `defcustom').
	(let ((paragraph-start (concat paragraph-start
                                       "\\|\\s-*[\(;:\"]")))
          (fill-paragraph justify))

      ;; Narrow to include only the comment, and then fill the region.
      (save-excursion
	(save-restriction
	  (beginning-of-line)
	  (narrow-to-region
	   ;; Find the first line we should include in the region to fill.
	   (save-excursion
	     (while (and (zerop (forward-line -1))
			 (looking-at "^[ \t]*;")))
	     ;; We may have gone too far.  Go forward again.
	     (or (looking-at ".*;")
		 (forward-line 1))
	     (point))
	   ;; Find the beginning of the first line past the region to fill.
	   (save-excursion
	     (while (progn (forward-line 1)
			   (looking-at "^[ \t]*;")))
	     (point)))

	  ;; Lines with only semicolons on them can be paragraph boundaries.
	  (let* ((paragraph-start (concat paragraph-start "\\|[ \t;]*$"))
		 (paragraph-separate (concat paragraph-start "\\|[ \t;]*$"))
		 (paragraph-ignore-fill-prefix nil)
		 (fill-prefix comment-fill-prefix)
		 (after-line (if has-code-and-comment
				 (save-excursion
				   (forward-line 1) (point))))
		 (end (progn
			(forward-paragraph)
			(or (bolp) (newline 1))
			(point)))
		 ;; If this comment starts on a line with code,
		 ;; include that like in the filling.
		 (beg (progn (backward-paragraph)
			     (if (eq (point) after-line)
				 (forward-line -1))
			     (point))))
	    (fill-region-as-paragraph beg end
				      justify nil
				      (save-excursion
					(goto-char beg)
					(if (looking-at fill-prefix)
					    nil
					  (re-search-forward comment-start-skip)
					  (point))))))))
    t))

(defun enki-indent-code-rigidly (start end arg &optional nochange-regexp)
  "Indent all lines of code, starting in the region, sideways by ARG columns.
Does not affect lines starting inside comments or strings, assuming that
the start of the region is not inside them.

Called from a program, takes args START, END, COLUMNS and NOCHANGE-REGEXP.
The last is a regexp which, if matched at the beginning of a line,
means don't indent that line."
  (interactive "r\np")
  (let (state)
    (save-excursion
      (goto-char end)
      (setq end (point-marker))
      (goto-char start)
      (or (bolp)
	  (setq state (parse-partial-sexp (point)
					  (progn
					    (forward-line 1) (point))
					  nil nil state)))
      (while (< (point) end)
	(or (car (nthcdr 3 state))
	    (and nochange-regexp
		 (looking-at nochange-regexp))
	    ;; If line does not start in string, indent it
	    (let ((indent (current-indentation)))
	      (delete-region (point) (progn (skip-chars-forward " \t") (point)))
	      (or (eolp)
		  (indent-to (max 0 (+ indent arg)) 0))))
	(setq state (parse-partial-sexp (point)
					(progn
					  (forward-line 1) (point))
					nil nil state))))))



(provide 'enki-mode)

;;; enki-mode.el ends here
