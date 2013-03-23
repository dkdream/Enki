;; -*- Mode: Emacs-Lisp -*-

;; http://zvon.org/other/elisp/Output/SEC337.html#SEC340
(defconst enki-font-lock-keywords-1
  `(
    ;; '(' <word.1> '(' <word.2> <3...> ')'
    (,(concat
       "(" (regexp-opt
            '("define" "macro") t)
       "\\>[ \t']*(\\(\\sw+\\)\\([^\)]*\\))")
     (1 font-lock-keyword-face)
     (2 font-lock-function-name-face t t)
     (3 font-lock-type-face t t))

    ;; '(' <word.1> '(' <2...> ')'
    (,(concat
       "(" (regexp-opt
            '("lambda") t)
       "\\>[ \t']*(\\([^\)]*\\)?)")
     (1 font-lock-keyword-face)
     (2 font-lock-type-face t t))

    ;; '(' <word.1> <word.2> ...
    (,(concat
       "(" (regexp-opt
            '("lambda") t)
       "\\>[ \t']*\\(\\sw+\\)?")
     (1 font-lock-keyword-face)
     (2 font-lock-type-face t t))
    
    ;; '(' <word.1> <word.2> ... 
    (,(concat
       "(" (regexp-opt
            '("let" "let*" "letrec") t)
       "\\>[ \t']*\\(\\sw+\\)?")
     (1 font-lock-keyword-face)
     (2 font-lock-function-name-face t t))
    
    ;; '(' <word>
    (,(concat
       "(" (regexp-opt
            '("if" "when" "and" "or" "unless" "begin"
              "let" "let*" "letrec"
              "define" "set" "macro"
              "delay" "force" "require"
              "type-of") t)
       "\\>") (1 font-lock-keyword-face))
        
    ;; '(' <word> ...
    (,(concat
       "(" (regexp-opt
            '("abort" "assert" "warn" "error" "signal") t)
       "\\>") (1 font-lock-warning-face))
    
    ;; '`' <word> tend to be symbol names.
    ("`\\(\\sw\\sw+\\)" (1 font-lock-constant-face append))
    
    ;; '\'' <word> tend to be symbol names.
    ("'\\(\\sw\\sw+\\)" (1 font-lock-constant-face append))
    
    ;; '.' <word>
    ("\\.\\(\\sw\\sw+\\)" (1 font-lock-constant-face append))
    
    ;; `&' <word> keywords as types.
    ("\\&\\(\\sw\\sw+\\)" (1 font-lock-type-face append))
    )
  "Gaudy level highlighting for Enki modes.")

(defvar enki-mode-abbrev-table nil)

(define-abbrev-table 'enki-mode-abbrev-table ())

;;(make-syntax-table) -> (make-char-table 'syntax-table nil)
(defvar enki-mode-syntax-table
  (let ((table (make-char-table 'syntax-table nil)))
    (let ((i 0))
      (while (< i ?\s)
	(modify-syntax-entry i "    " table) ;; 00-1f
	(setq i (1+ i)))
      ;; symbols
      (modify-syntax-entry ?! "_   " table) ;; symbol constituent
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
      (modify-syntax-entry ?@ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?^ "_   " table) ;; symbol constituent
      (modify-syntax-entry ?| "_   " table) ;; symbol constituent
      (modify-syntax-entry ?~ "_   " table) ;; symbol constituent

      ;; whitespace
      (modify-syntax-entry ?\s "    " table) ;; whitespace
      (modify-syntax-entry ?\t "    " table) ;; whitespace
      (modify-syntax-entry ?\f "    " table) ;; whitespace

      ;; comment
      (modify-syntax-entry ?\# "<   " table) ;; comment starter
      (modify-syntax-entry ?\n ">   " table) ;; comment ender

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
      (modify-syntax-entry ?\" "\"   " table) ;; string quote
      (modify-syntax-entry ?\\ "\\   " table) ;; escape character

      ;; expression delimiters
      (modify-syntax-entry ?\( "()  " table) ;; open  (
      (modify-syntax-entry ?\) ")(  " table) ;; close )
      (modify-syntax-entry ?\[ "(]  " table) ;; open  [
      (modify-syntax-entry ?\] ")[  " table) ;; close ]
      (modify-syntax-entry ?\{ "(}  " table) ;; open  {
      (modify-syntax-entry ?\} "){  " table) ;; close }
      )
    table))

(defvar enki-doc-string-elt-property 'doc-string-elt
  "The symbol property that holds the docstring position info.")

(defun enki-font-lock-syntactic-face-function (state)
  (if (nth 3 state)
      ;; This might be a (doc)string or a |...| symbol.
      (let ((startpos (nth 8 state)))
        (if (eq (char-after startpos) ?|)
            ;; This is not a string, but a |...| symbol.
            nil
          (let* ((listbeg (nth 1 state))
                 (firstsym (and listbeg
                                (save-excursion
                                  (goto-char listbeg)
                                  (and (looking-at "([ \t\n]*\\(\\(\\sw\\|\\s_\\)+\\)")
                                       (match-string 1)))))
                 (docelt (and firstsym (get (intern-soft firstsym)
                                            enki-doc-string-elt-property))))
            (if (and docelt
                     ;; It's a string in a form that can have a docstring.
                     ;; Check whether it's in docstring position.
                     (save-excursion
                       (when (functionp docelt)
                         (goto-char (match-end 1))
                         (setq docelt (funcall docelt)))
                       (goto-char listbeg)
                       (forward-char 1)
                       (condition-case nil
                           (while (and (> docelt 0) (< (point) startpos)
                                       (progn (forward-sexp 1) t))
                             (setq docelt (1- docelt)))
                         (error nil))
                       (and (zerop docelt) (<= (point) startpos)
                            (progn (forward-comment (point-max)) t)
                            (= (point) (nth 8 state)))))
                font-lock-doc-face
              font-lock-string-face))))
    font-lock-comment-face))

(defun enki-mode-variables ()
  "Common initialization routine for enki modes."
  (set-syntax-table enki-mode-syntax-table)
  (setq local-abbrev-table enki-mode-abbrev-table)

  (make-local-variable 'paragraph-ignore-fill-prefix)
  (setq paragraph-ignore-fill-prefix t)

  (make-local-variable 'fill-paragraph-function)
  (setq fill-paragraph-function 'enki-fill-paragraph)

  ;; Adaptive fill mode gets the fill wrong for a one-line paragraph made of
  ;; a single docstring.  Let's fix it here.
  (set (make-local-variable 'adaptive-fill-function)
       (lambda () (if (looking-at "\\s-+\"[^\n\"]+\"\\s-*$") "")))

  ;; Adaptive fill mode gets in the way of auto-fill,
  ;; and should make no difference for explicit fill
  ;; because enki-fill-paragraph should do the job.
  ;;(set (make-local-variable 'adaptive-fill-mode) nil)
  (make-local-variable 'indent-line-function)
  (setq indent-line-function 'enki-indent-line)

  (make-local-variable 'parse-sexp-ignore-comments)
  (setq parse-sexp-ignore-comments t)

  (make-local-variable 'comment-start)
  (setq comment-start "#")

  ;; Look within the line for a # following an even number of backslashes
  ;; after either a non-backslash or the line beginning.
  (make-local-variable 'comment-start-skip)
  (setq comment-start-skip "\\(\\(^\\|[^\\\\\n]\\)\\(\\\\\\\\\\)*\\)#+ *")

  ;; Font lock mode uses this only when it KNOWS a comment is starting.
  (make-local-variable 'font-lock-comment-start-skip)
  (setq font-lock-comment-start-skip "#+ *")

  ;; default to `##' in comment-region
  (make-local-variable 'comment-add)
  (setq comment-add 1)

  (make-local-variable 'comment-column)
  (setq comment-column 40)

  ;; Don't get confused by `#' in doc strings when paragraph-filling.
  (set (make-local-variable 'comment-use-global-state) t)

  (make-local-variable 'multibyte-syntax-as-symbol)
  (setq multibyte-syntax-as-symbol t)

  (set (make-local-variable 'syntax-begin-function) 'beginning-of-defun)
  (setq font-lock-defaults
	`((enki-font-lock-keywords enki-font-lock-keywords-1)      ;; keywords
	  nil                            ;; keywords-only 
          nil                            ;; case-fold
          (("+-*/.<>=!?$%_&~^:@" . "w")) ;; syntax-alist 
          nil                            ;; syntax-begin
	  (font-lock-mark-block-function . mark-defun) ;; other-vars...
	  (font-lock-syntactic-face-function . enki-font-lock-syntactic-face-function))))

(defvar enki-mode-shared-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\e\C-q" 'enki-indent-sexp)
    (define-key map "\177" 'backward-delete-char-untabify)
    ;; This gets in the way when viewing a Enki file in view-mode.  As
    ;; long as [backspace] is mapped into DEL via the
    ;; function-key-map, this should remain disabled!!
    ;;;(define-key map [backspace] 'backward-delete-char-untabify)
    map)
  "Keymap for commands shared by all sorts of Enki modes.")

(defvar enki-mode-map
  (let ((map (make-sparse-keymap))
	(menu-map (make-sparse-keymap "Enki")))
    (set-keymap-parent map enki-mode-shared-map)
    (define-key map "\C-c\C-z" 'run-enki)
    (define-key map [menu-bar enki] (cons "Enki" menu-map))
    (define-key menu-map [run-enki]
      '(menu-item "Run inferior Enki" run-enki
		  :help "Run an inferior Enki process, input and output via buffer `*inferior-enki*'"))
    (define-key menu-map [ev-def]
      '(menu-item "Eval defun" enki-eval-defun
		  :help "Send the current defun to the Enki process made by M-x run-enki"))
    (define-key menu-map [ind-sexp]
      '(menu-item "Indent sexp" enki-indent-sexp
		  :help "Indent each line of the list starting just after point"))
    map)
  "Keymap for ordinary Enki mode.
All commands in `enki-mode-shared-map' are inherited by this map.")

(defun enki-mode ()
  "Major mode for editing Enki code for Enkis other than GNU Emacs Enki.
Commands:
Delete converts tabs to spaces as it moves back.
Blank lines separate paragraphs.  Semicolons start comments.

\\{enki-mode-map}
Note that `run-enki' may be used either to start an inferior Enki job
or to switch back to an existing one.

Entry to this mode calls the value of `enki-mode-hook'
if that value is non-nil."
  (interactive)
  (kill-all-local-variables)
  (use-local-map enki-mode-map)
  (setq major-mode 'enki-mode)
  (setq mode-name "Enki")
  (enki-mode-variables)
  (make-local-variable 'comment-start-skip)
  (setq comment-start-skip "\\(\\(^\\|[^\\\\\n]\\)\\(\\\\\\\\\\)*\\)\\(#+\\|#!\\) *")
  (setq imenu-case-fold-search t)
  (set-syntax-table enki-mode-syntax-table)
  (run-mode-hooks 'enki-mode-hook))

(put 'enki-mode 'find-tag-default-function 'enki-find-tag-default)

(defun enki-find-tag-default ()
  (let ((default (find-tag-default)))
    (when (stringp default)
      (if (string-match ":+" default)
          (substring default (match-end 0))
	default))))

(defcustom enki-mode-hook nil
  "Hook run when entering Enki mode."
  :options '(imenu-add-menubar-index)
  :type 'hook
  :group 'enki)

(defcustom enki-indent-offset nil
  "If non-nil, indent second line of expressions that many more columns."
  :group 'enki
  :type '(choice (const nil) integer))

(defcustom enki-indent-function 'enki-indent-function
  "A function to be called by `calculate-enki-indent'.
It indents the arguments of a Enki function call.  This function
should accept two arguments: the indent-point, and the
`parse-partial-sexp' state at that position.  One option for this
function is `common-enki-indent-function'."
  :type 'function
  :group 'enki)

(defun enki-indent-line (&optional whole-exp)
  "Indent current line as Enki code.
With argument, indent any additional lines of the same expression
rigidly along with this one."
  (interactive "P")
  (message "CALLING INDENT-LINE %s\n" whole-exp)
  (let ((indent (calculate-enki-indent)) shift-amt end
	(pos (- (point-max) (point)))
	(beg (progn (beginning-of-line) (point))))
    (skip-chars-forward " \t")
    (if (or (null indent) (looking-at "\\s<\\s<\\s<"))
	;; Don't alter indentation of a ;;; comment line
	;; or a line that starts in a string.
	(goto-char (- (point-max) pos))
      (if (and (looking-at "\\s<") (not (looking-at "\\s<\\s<")))
	  ;; Single-semicolon comment lines should be indented
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
	   (indent-code-rigidly beg end shift-amt)))))

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
  (message "CALLING CALCULATE-ENKI-INDENT %s\n" parse-start)
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
              ;; in this case calculate-enki-indent-last-sexp is not nil
              (calculate-enki-indent-last-sexp
               (or
                ;; try to align the parameters of a known function
                (and enki-indent-function
                     (not retry)
                     (funcall enki-indent-function indent-point state))
                ;; If the function has no special alignment
		;; or it does not apply to this argument,
		;; try to align a constant-symbol under the last
                ;; preceding constant symbol, if there is such one of
                ;; the last 2 preceding symbols, in the previous
                ;; uncommented line.
                (and (save-excursion
                       (goto-char indent-point)
                       (skip-chars-forward " \t")
                       (looking-at ":"))
                     ;; The last sexp may not be at the indentation
                     ;; where it begins, so find that one, instead.
                     (save-excursion
                       (goto-char calculate-enki-indent-last-sexp)
		       ;; Handle prefix characters and whitespace
		       ;; following an open paren.  (Bug#1012)
                       (backward-prefix-chars)
                       (while (and (not (looking-back "^[ \t]*\\|([ \t]+"))
                                   (or (not containing-sexp)
                                       (< (1+ containing-sexp) (point))))
                         (forward-sexp -1)
                         (backward-prefix-chars))
                       (setq calculate-enki-indent-last-sexp (point)))
                     (> calculate-enki-indent-last-sexp
                        (save-excursion
                          (goto-char (1+ containing-sexp))
                          (parse-partial-sexp (point) calculate-enki-indent-last-sexp 0 t)
                          (point)))
                     (let ((parse-sexp-ignore-comments t)
                           indent)
                       (goto-char calculate-enki-indent-last-sexp)
                       (or (and (looking-at ":")
                                (setq indent (current-column)))
                           (and (< (save-excursion (beginning-of-line) (point))
                                   (prog2 (backward-sexp) (point)))
                                (looking-at ":")
                                (setq indent (current-column))))
                       indent))
                ;; another symbols or constants not preceded by a constant
                ;; as defined above.
                normal-indent))
              ;; in this case calculate-enki-indent-last-sexp is nil
              (desired-indent)
              (t
               normal-indent))))))

(defun enki-indent-function (indent-point state)
  "This function is the normal value of the variable `enki-indent-function'.
It is used when indenting a line within a function call, to see if the
called function says anything special about how to indent the line.

INDENT-POINT is the position where the user typed TAB, or equivalent.
Point is located at the point to indent under (for default indentation);
STATE is the `parse-partial-sexp' state for that position.

If the current line is in a call to a Enki function
which has a non-nil property `enki-indent-function',
that specifies how to do the indentation.  The property value can be
* `defun', meaning indent `defun'-style;
* an integer N, meaning indent the first N arguments specially
  like ordinary function arguments and then indent any further
  arguments like a body;
* a function to call just as this function was called.
  If that function returns nil, that means it doesn't specify
  the indentation.

This function also returns nil meaning don't specify the indentation."
  (let ((normal-indent (current-column)))
    (message "CALLING INDENT-FUNCTION %s - %s\n" indent-point state)
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
;;	(setq method (or (get (intern-soft function) 'enki-indent-function)
;;			 (get (intern-soft function) 'enki-indent-hook)))
        (setq method (get (intern-soft function) 'enki-indent-function))
        (message "method is %s\n", method)
        (message "function is %s\n", function)
	(cond ((or (eq method 'defun)
		   (and (null method)
			(> (length function) 3)
			(string-match "def" function)))
                 (enki-indent-defform state indent-point))
	      ((integerp method)
	       (enki-indent-specform method state indent-point normal-indent))
	      (method 
               (funcall method indent-point state)))))))

(defcustom enki-body-indent 4
  "Number of columns to indent the second line of a (def...)' form."
  :group 'enki
  :type 'integer)

(put 'enki-body-indent 'safe-local-variable 'integerp)

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


;; (put 'progn 'enki-indent-function 0), say, causes progn to be indented
;; like defun if the first form is placed on the next line, otherwise
;; it is indented like any other form (i.e. forms line up under first).

;;(put 'lambda 'enki-indent-function 1)
;;(put 'let 'enki-indent-function 1)
;;(put 'let* 'enki-indent-function 1)
;;(put 'letrec 'enki-indent-function 1)
;;(put 'while 'enki-indent-function 1)
;;(put 'if 'enki-indent-function 2)
;;(put 'when 'enki-indent-function 1)
;;(put 'unless 'enki-indent-function 1)

(defun enki-indent-sexp (&optional endpos)
  "Indent each line of the list starting just after point.
If optional arg ENDPOS is given, indent each line, stopping when
ENDPOS is encountered."
  (interactive)
  (message "CALLING INDENT-SEXP FUNCTION %s\n" endpos)
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
	(forward-line 1)
	;; Decide whether to exit.
	(if endpos
	    ;; If we have already reached the specified end,
	    ;; give up and do not reindent this line.
	    (if (<= endpos (point))
		(setq outer-loop-done t))
	  ;; If no specified end, we are done if we have finished one sexp.
	  (if (<= next-depth 0)
	      (setq outer-loop-done t)))
	(unless outer-loop-done
	  (while (> last-depth next-depth)
	    (setq indent-stack (cdr indent-stack)
		  last-depth (1- last-depth)))
	  (while (< last-depth next-depth)
	    (setq indent-stack (cons nil indent-stack)
		  last-depth (1+ last-depth)))
	  ;; Now indent the next line according
	  ;; to what we learned from parsing the previous one.
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

(add-to-list 'auto-mode-alist '("\\.ea\\'" . enki-mode))

(provide 'enki-mode)
