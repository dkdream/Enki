;; -*- Mode: Emacs-Lisp -*-

(require 'comint)
(require 'enki-mode)

(defgroup enki-shell nil
  "Run an outside Enki in an Emacs buffer."
  :group 'enki
  :version "22.1")

(defcustom enki-shell-filter-regexp
  "\\`\\s *\\(:\\(\\w\\|\\s_\\)\\)?\\s *\\'"
  "What not to save on inferior Enki's input history.
Input matching this regexp is not saved on the input history in Inferior Enki
mode.  Default is whitespace followed by 0 or 1 single-letter colon-keyword
\(as in :a, :c, etc.)"
  :type 'regexp
  :group 'enki-shell)

(defvar enki-shell-mode-map
  (let ((map (copy-keymap comint-mode-map)))
    (set-keymap-parent map enki-mode-shared-map)
    (define-key map "\C-x\C-e" 'enki-eval-last-sexp)
    (define-key map "\C-c\C-l" 'enki-load-file)
    (define-key map "\C-c\C-k" 'enki-compile-file)
    (define-key map "\C-c\C-a" 'enki-show-arglist)
    (define-key map "\C-c\C-d" 'enki-describe-sym)
    (define-key map "\C-c\C-f" 'enki-show-function-documentation)
    (define-key map "\C-c\C-v" 'enki-show-variable-documentation)
    map))

;;; These commands augment Enki mode, so you can process Enki code in
;;; the source files.
(define-key enki-mode-map "\M-\C-x"  'enki-eval-defun)     ; Gnu convention
(define-key enki-mode-map "\C-x\C-e" 'enki-eval-last-sexp) ; Gnu convention
(define-key enki-mode-map "\C-c\C-e" 'enki-eval-defun)
(define-key enki-mode-map "\C-c\C-r" 'enki-eval-region)
(define-key enki-mode-map "\C-c\C-c" 'enki-compile-defun)
(define-key enki-mode-map "\C-c\C-z" 'switch-to-enki)
(define-key enki-mode-map "\C-c\C-l" 'enki-load-file)
(define-key enki-mode-map "\C-c\C-k" 'enki-compile-file)  ; "kompile" file
(define-key enki-mode-map "\C-c\C-a" 'enki-show-arglist)
(define-key enki-mode-map "\C-c\C-d" 'enki-describe-sym)
(define-key enki-mode-map "\C-c\C-f" 'enki-show-function-documentation)
(define-key enki-mode-map "\C-c\C-v" 'enki-show-variable-documentation)


;;; This function exists for backwards compatibility.
;;; Previous versions of this package bound commands to C-c <letter>
;;; bindings, which is not allowed by the gnumacs standard.

;;;  "This function binds many enki-shell commands to C-c <letter> bindings,
;;;where they are more accessible. C-c <letter> bindings are reserved for the
;;;user, so these bindings are non-standard. If you want them, you should
;;;have this function called by the enki-shell-load-hook:
;;;  (add-hook 'enki-shell-load-hook 'enki-shell-install-letter-bindings)
;;;You can modify this function to install just the bindings you want."
(defun enki-shell-install-letter-bindings ()
  (define-key enki-mode-map "\C-ce" 'enki-eval-defun-and-go)
  (define-key enki-mode-map "\C-cr" 'enki-eval-region-and-go)
  (define-key enki-mode-map "\C-cc" 'enki-compile-defun-and-go)
  (define-key enki-mode-map "\C-cz" 'switch-to-enki)
  (define-key enki-mode-map "\C-cl" 'enki-load-file)
  (define-key enki-mode-map "\C-ck" 'enki-compile-file)
  (define-key enki-mode-map "\C-ca" 'enki-show-arglist)
  (define-key enki-mode-map "\C-cd" 'enki-describe-sym)
  (define-key enki-mode-map "\C-cf" 'enki-show-function-documentation)
  (define-key enki-mode-map "\C-cv" 'enki-show-variable-documentation)

  (define-key enki-shell-mode-map "\C-cl" 'enki-load-file)
  (define-key enki-shell-mode-map "\C-ck" 'enki-compile-file)
  (define-key enki-shell-mode-map "\C-ca" 'enki-show-arglist)
  (define-key enki-shell-mode-map "\C-cd" 'enki-describe-sym)
  (define-key enki-shell-mode-map "\C-cf" 'enki-show-function-documentation)
  (define-key enki-shell-mode-map "\C-cv"
    'enki-show-variable-documentation))

(defcustom enki-shell-program "enki"
  "Program name for invoking an inferior Enki in Inferior Enki mode."
  :type 'string
  :group 'enki-shell)

(defcustom enki-shell-load-command "(load \"%s\")\n"
  "Format-string for building a Enki expression to load a file.
This format string should use `%s' to substitute a file name
and should result in a Enki expression that will command the inferior Enki
to load that file.  The default works acceptably on most Enkis.
The string \"(progn (load \\\"%s\\\" :verbose nil :print t) (values))\\n\"
produces cosmetically superior output for this application,
but it works only in Common Enki."
  :type 'string
  :group 'enki-shell)

(defcustom enki-shell-prompt "^[^> \n]*>+:? *"
  "Regexp to recognize prompts in the Inferior Enki mode.
Defaults to \"^[^> \\n]*>+:? *\", which works pretty good for Lucid, kcl,
and franz.  This variable is used to initialize `comint-prompt-regexp' in the
Inferior Enki buffer.

This variable is only used if the variable
`comint-use-prompt-regexp' is non-nil.

More precise choices:
Lucid Common Enki: \"^\\\\(>\\\\|\\\\(->\\\\)+\\\\) *\"
franz: \"^\\\\(->\\\\|<[0-9]*>:\\\\) *\"
kcl: \"^>+ *\""
  :type 'regexp
  :group 'enki-shell)

(defvar enki-shell-buffer nil "*The current enki-shell process buffer.

MULTIPLE PROCESS SUPPORT
===========================================================================
To run multiple Enki processes, you start the first up
with \\[enki-shell].  It will be in a buffer named `*enki-shell*'.
Rename this buffer with \\[rename-buffer].  You may now start up a new
process with another \\[enki-shell].  It will be in a new buffer,
named `*enki-shell*'.  You can switch between the different process
buffers with \\[switch-to-buffer].

Commands that send text from source buffers to Enki processes --
like `enki-eval-defun' or `enki-show-arglist' -- have to choose a process
to send to, when you have more than one Enki process around.  This
is determined by the global variable `enki-shell-buffer'.  Suppose you
have three inferior Enkis running:
    Buffer              Process
    foo                 enki-shell
    bar                 enki-shell<2>
    *enki-shell*     enki-shell<3>
If you do a \\[enki-eval-defun] command on some Enki source code,
what process do you send it to?

- If you're in a process buffer (foo, bar, or *enki-shell*),
  you send it to that process.
- If you're in some other buffer (e.g., a source file), you
  send it to the process attached to buffer `enki-shell-buffer'.
This process selection is performed by function `enki-shell-proc'.

Whenever \\[enki-shell] fires up a new process, it resets
`enki-shell-buffer' to be the new process's buffer.  If you only run
one process, this does the right thing.  If you run multiple
processes, you can change `enki-shell-buffer' to another process
buffer with \\[set-variable].")

(defvar enki-shell-mode-hook '()
  "Hook for customizing Inferior Enki mode.")

(put 'enki-shell-mode 'mode-class 'special)

(define-derived-mode enki-shell-mode comint-mode "Inferior Enki"
  "Major mode for interacting with an inferior Enki process.
Runs a Enki interpreter as a subprocess of Emacs, with Enki I/O through an
Emacs buffer.  Variable `enki-shell-program' controls which Enki interpreter
is run.  Variables `enki-shell-prompt', `enki-shell-filter-regexp' and
`enki-shell-load-command' can customize this mode for different Enki
interpreters.

For information on running multiple processes in multiple buffers, see
documentation for variable `enki-shell-buffer'.

\\{enki-shell-mode-map}

Customization: Entry to this mode runs the hooks on `comint-mode-hook' and
`enki-shell-mode-hook' (in that order).

You can send text to the inferior Enki process from other buffers containing
Enki source.
    `switch-to-enki' switches the current buffer to the Enki process buffer.
    `enki-eval-defun' sends the current defun to the Enki process.
    `enki-compile-defun' compiles the current defun.
    `enki-eval-region' sends the current region to the Enki process.
    `enki-compile-region' compiles the current region.

    Prefixing the enki-eval/compile-defun/region commands with
    a \\[universal-argument] causes a switch to the Enki process buffer after sending
    the text.

Commands:\\<enki-shell-mode-map>
\\[comint-send-input] after the end of the process' output sends the text from the
    end of process to point.
\\[comint-send-input] before the end of the process' output copies the sexp ending at point
    to the end of the process' output, and sends it.
\\[comint-copy-old-input] copies the sexp ending at point to the end of the process' output,
    allowing you to edit it before sending it.
If `comint-use-prompt-regexp' is nil (the default), \\[comint-insert-input] on old input
   copies the entire old input to the end of the process' output, allowing
   you to edit it before sending it.  When not used on old input, or if
   `comint-use-prompt-regexp' is non-nil, \\[comint-insert-input] behaves according to
   its global binding.
\\[backward-delete-char-untabify] converts tabs to spaces as it moves back.
\\[enki-indent-line] indents for Enki; with argument, shifts rest
    of expression rigidly with the current line.
\\[indent-sexp] does \\[enki-indent-line] on each line starting within following expression.
Paragraphs are separated only by blank lines.  Semicolons start comments.
If you accidentally suspend your process, use \\[comint-continue-subjob]
to continue it."
  (setq comint-prompt-regexp enki-shell-prompt)
  (setq mode-line-process '(":%s"))
  (enki-mode-variables t)
  (setq comint-get-old-input (function enki-get-old-input))
  (setq comint-input-filter (function enki-input-filter)))

(defun enki-get-old-input ()
  "Return a string containing the sexp ending at point."
  (save-excursion
    (let ((end (point)))
      (backward-sexp)
      (buffer-substring (point) end))))

(defun enki-input-filter (str)
  "t if STR does not match `enki-shell-filter-regexp'."
  (not (string-match enki-shell-filter-regexp str)))

;;;###autoload
(defun enki-shell (cmd)
  "Run an inferior Enki process, input and output via buffer `*enki-shell*'.
If there is a process already running in `*enki-shell*', just switch
to that buffer.
With argument, allows you to edit the command line (default is value
of `enki-shell-program').  Runs the hooks from
`enki-shell-mode-hook' (after the `comint-mode-hook' is run).
\(Type \\[describe-mode] in the process buffer for a list of commands.)"
  (interactive (list (if current-prefix-arg
			 (read-string "Run enki: " enki-shell-program)
		       enki-shell-program)))
  (if (not (comint-check-proc "*enki-shell*"))
      (let ((cmdlist (split-string cmd)))
	(set-buffer (apply (function make-comint)
			   "enki-shell" (car cmdlist) nil (cdr cmdlist)))
	(enki-shell-mode)))
  (setq enki-shell-buffer "*enki-shell*")
  (pop-to-buffer-same-window "*enki-shell*"))

;;;###autoload
(defalias 'run-enki 'enki-shell)

(defun enki-eval-region (start end &optional and-go)
  "Send the current region to the inferior Enki process.
Prefix argument means switch to the Enki buffer afterwards."
  (interactive "r\nP")
  (comint-send-region (enki-shell-proc) start end)
  (comint-send-string (enki-shell-proc) "\n")
  (if and-go (switch-to-enki t)))

(defun enki-compile-string (string)
  "Send the string to the inferior Enki process to be compiled and executed."
  (comint-send-string
   (enki-shell-proc)
   (format "(funcall (compile nil (lambda () %s)))\n" string)))

(defun enki-eval-string (string)
  "Send the string to the inferior Enki process to be executed."
  (comint-send-string (enki-shell-proc) (concat string "\n")))

(defun enki-do-defun (do-string do-region)
  "Send the current defun to the inferior Enki process.
The actually processing is done by `do-string' and `do-region'
 which determine whether the code is compiled before evaluation.
DEFVAR forms reset the variables to the init values."
  (save-excursion
    (end-of-defun)
    (skip-chars-backward " \t\n\r\f") ;  Makes allegro happy
    (let ((end (point)) (case-fold-search t))
      (beginning-of-defun)
      (if (looking-at "(defvar")
          (funcall do-string
                   ;; replace `defvar' with `defparameter'
                   (concat "(defparameter "
                           (buffer-substring-no-properties (+ (point) 7) end)
                           "\n"))
        (funcall do-region (point) end)))))

(defun enki-eval-defun (&optional and-go)
  "Send the current defun to the inferior Enki process.
DEFVAR forms reset the variables to the init values.
Prefix argument means switch to the Enki buffer afterwards."
  (interactive "P")
  (enki-do-defun 'enki-eval-string 'enki-eval-region)
  (if and-go (switch-to-enki t)))

(defun enki-eval-last-sexp (&optional and-go)
  "Send the previous sexp to the inferior Enki process.
Prefix argument means switch to the Enki buffer afterwards."
  (interactive "P")
  (enki-eval-region (save-excursion (backward-sexp) (point)) (point) and-go))

(defun enki-compile-region (start end &optional and-go)
  "Compile the current region in the inferior Enki process.
Prefix argument means switch to the Enki buffer afterwards."
  (interactive "r\nP")
  (enki-compile-string (buffer-substring-no-properties start end))
  (if and-go (switch-to-enki t)))

(defun enki-compile-defun (&optional and-go)
  "Compile the current defun in the inferior Enki process.
DEFVAR forms reset the variables to the init values.
Prefix argument means switch to the Enki buffer afterwards."
  (interactive "P")
  (enki-do-defun 'enki-compile-string 'enki-compile-region)
  (if and-go (switch-to-enki t)))

(defun switch-to-enki (eob-p)
  "Switch to the inferior Enki process buffer.
With argument, positions cursor at end of buffer."
  (interactive "P")
  (if (get-buffer-process enki-shell-buffer)
      (let ((pop-up-frames
	     ;; Be willing to use another frame
	     ;; that already has the window in it.
	     (or pop-up-frames
		 (get-buffer-window enki-shell-buffer t))))
	(pop-to-buffer enki-shell-buffer))
      (run-enki enki-shell-program))
  (when eob-p
	 (push-mark)
    (goto-char (point-max))))


;;; Now that enki-compile/eval-defun/region takes an optional prefix arg,
;;; these commands are redundant. But they are kept around for the user
;;; to bind if he wishes, for backwards functionality, and because it's
;;; easier to type C-c e than C-u C-c C-e.

(defun enki-eval-region-and-go (start end)
  "Send the current region to the inferior Enki, and switch to its buffer."
  (interactive "r")
  (enki-eval-region start end t))

(defun enki-eval-defun-and-go ()
  "Send the current defun to the inferior Enki, and switch to its buffer."
  (interactive)
  (enki-eval-defun t))

(defun enki-compile-region-and-go (start end)
  "Compile the current region in the inferior Enki, and switch to its buffer."
  (interactive "r")
  (enki-compile-region start end t))

(defun enki-compile-defun-and-go ()
  "Compile the current defun in the inferior Enki, and switch to its buffer."
  (interactive)
  (enki-compile-defun t))

(defvar enki-prev-l/c-dir/file nil
  "Record last directory and file used in loading or compiling.
This holds a cons cell of the form `(DIRECTORY . FILE)'
describing the last `enki-load-file' or `enki-compile-file' command.")

(defcustom enki-source-modes '(enki-mode)
  "Used to determine if a buffer contains Enki source code.
If it's loaded into a buffer that is in one of these major modes, it's
considered a Enki source file by `enki-load-file' and `enki-compile-file'.
Used by these commands to determine defaults."
  :type '(repeat symbol)
  :group 'enki-shell)

(defun enki-load-file (file-name)
  "Load a Enki file into the inferior Enki process."
  (interactive (comint-get-source "Load Enki file: " enki-prev-l/c-dir/file
				  enki-source-modes nil)) ; nil because LOAD
					; doesn't need an exact name
  (comint-check-source file-name) ; Check to see if buffer needs saved.
  (setq enki-prev-l/c-dir/file (cons (file-name-directory    file-name)
				     (file-name-nondirectory file-name)))
  (comint-send-string (enki-shell-proc)
		      (format enki-shell-load-command file-name))
  (switch-to-enki t))


(defun enki-compile-file (file-name)
  "Compile a Enki file in the inferior Enki process."
  (interactive (comint-get-source "Compile Enki file: " enki-prev-l/c-dir/file
				  enki-source-modes nil)) ; nil = don't need
					; suffix .enki
  (comint-check-source file-name) ; Check to see if buffer needs saved.
  (setq enki-prev-l/c-dir/file (cons (file-name-directory    file-name)
				     (file-name-nondirectory file-name)))
  (comint-send-string (enki-shell-proc) (concat "(compile-file \""
						   file-name
						   "\"\)\n"))
  (switch-to-enki t))



;;; Documentation functions: function doc, var doc, arglist, and
;;; describe symbol.
;;; ===========================================================================

;;; Command strings
;;; ===============

(defvar enki-function-doc-command
  "(let ((fn '%s))
     (format t \"Documentation for ~a:~&~a\"
	     fn (documentation fn 'function))
     (values))\n"
  "Command to query inferior Enki for a function's documentation.")

(defvar enki-var-doc-command
  "(let ((v '%s))
     (format t \"Documentation for ~a:~&~a\"
	     v (documentation v 'variable))
     (values))\n"
  "Command to query inferior Enki for a variable's documentation.")

(defvar enki-arglist-command
  "(let ((fn '%s))
     (format t \"Arglist for ~a: ~a\" fn (arglist fn))
     (values))\n"
  "Command to query inferior Enki for a function's arglist.")

(defvar enki-describe-sym-command
  "(describe '%s)\n"
  "Command to query inferior Enki for a variable's documentation.")


;;; Ancillary functions
;;; ===================

;;; Reads a string from the user.
(defun enki-symprompt (prompt default)
  (list (let* ((prompt (if default
			   (format "%s (default %s): " prompt default)
			 (concat prompt ": ")))
	       (ans (read-string prompt)))
	  (if (zerop (length ans)) default ans))))


;;; Adapted from function-called-at-point in help.el.
(defun enki-fn-called-at-pt ()
  "Returns the name of the function called in the current call.
The value is nil if it can't find one."
  (condition-case nil
      (save-excursion
	(save-restriction
	  (narrow-to-region (max (point-min) (- (point) 1000)) (point-max))
	  (backward-up-list 1)
	  (forward-char 1)
	  (let ((obj (read (current-buffer))))
	    (and (symbolp obj) obj))))
    (error nil)))


;;; Adapted from variable-at-point in help.el.
(defun enki-var-at-pt ()
  (condition-case ()
      (save-excursion
	(forward-sexp -1)
	(skip-chars-forward "'")
	(let ((obj (read (current-buffer))))
	  (and (symbolp obj) obj)))
    (error nil)))


;;; Documentation functions: fn and var doc, arglist, and symbol describe.
;;; ======================================================================

(defun enki-show-function-documentation (fn)
  "Send a command to the inferior Enki to give documentation for function FN.
See variable `enki-function-doc-command'."
  (interactive (enki-symprompt "Function doc" (enki-fn-called-at-pt)))
  (comint-proc-query (enki-shell-proc)
		     (format enki-function-doc-command fn)))

(defun enki-show-variable-documentation (var)
  "Send a command to the inferior Enki to give documentation for function FN.
See variable `enki-var-doc-command'."
  (interactive (enki-symprompt "Variable doc" (enki-var-at-pt)))
  (comint-proc-query (enki-shell-proc) (format enki-var-doc-command var)))

(defun enki-show-arglist (fn)
  "Send a query to the inferior Enki for the arglist for function FN.
See variable `enki-arglist-command'."
  (interactive (enki-symprompt "Arglist" (enki-fn-called-at-pt)))
  (comint-proc-query (enki-shell-proc) (format enki-arglist-command fn)))

(defun enki-describe-sym (sym)
  "Send a command to the inferior Enki to describe symbol SYM.
See variable `enki-describe-sym-command'."
  (interactive (enki-symprompt "Describe" (enki-var-at-pt)))
  (comint-proc-query (enki-shell-proc)
		     (format enki-describe-sym-command sym)))

;;  "Returns the current inferior Enki process.
;; See variable `enki-shell-buffer'."
(defun enki-shell-proc ()
  (let ((proc (get-buffer-process (if (derived-mode-p 'enki-shell-mode)
				      (current-buffer)
				    enki-shell-buffer))))
    (or proc
	(error "No Enki subprocess; see variable `enki-shell-buffer'"))))


;;; Do the user's customization...
;;;===============================
(defvar enki-shell-load-hook nil
  "This hook is run when the library `enki-shell' is loaded.")

(run-hooks 'enki-shell-load-hook)

(provide 'enki-shell)

;;; enki-shell.el ends here
