;; -*- Mode: Emacs-Lisp -*-
(require 'scheme)

(defgroup enki nil
  "A major mode for editing enki files."
  :group 'languages)

(defcustom enki-mode-hook nil
  "Hook run at the end of the initialization of the enki mode."
  :type 'hook
  :group 'programming)

(defvar enki-font-lock-keywords
  `((,(concat "(" (regexp-opt
                   '(
                     "and"
                     "apply"
                     "assert"
                     "begin"
                     "define"
                     "delay"
                     "force"
                     "eval"
                     "if"
                     "lambda"
                     "let"
                     "macro"
                     "or"
                     "set"
                     "type-of"
                     "while"
                     ) t))
     1 font-lock-function-name-face)
    ;; named let
    ("(let\\s-+\\(\\sw+\\)" (1 font-lock-function-name-face))
    ;; functions
    ("(define\\s-+(?\\(\\sw+\\)" (1 font-lock-function-name-face))
    ;; datatypes
    ;; ("(datatype\\s-+\\(\\sw+\\)" (1 font-lock-type-face))
    ;; constructors - namespace:name
    ;;("\\<\\sw*:\\sw+\\>" (1 font-lock-type-face))
    ;; pattern matching - pattern -> expr 
    ;; ("->" (1 font-lock-function-name-face))
    )
  "notes.")

(defun enki-mode ()
  "Major mode for editing Enki code."
  (interactive)
  (kill-all-local-variables)
  (use-local-map scheme-mode-map)
  (setq major-mode 'enki-mode)
  (setq mode-name "Enki")
  (scheme-mode-variables)
  (setq font-lock-defaults '(enki-font-lock-keywords
                             nil t (("+-*/.<>=!?$%_&~^:" . "w") (?#. "w 14"))
                             beginning-of-defun
                             (font-lock-mark-block-function . mark-defun)
                             (font-lock-syntactic-face-function . scheme-font-lock-syntactic-face-function)
                             (parse-sexp-lookup-properties . t)
                             (font-lock-extra-managed-props syntax-table)))
  (run-hooks 'enki-mode-hook))

;; there's probably an easier way to do this, but I'd like to
;;  be able to make this smarter in the future.  Specifically
;;  it might be nice to leave a hanging -> at the end of a line,
;;  and have this function automatically indent the next line by 2.
;; See http://community.schemewiki.org/?emacs-indentation
(defun enki-match-indent (state indent-point normal-indent)
  (forward-char 1)
  (goto-char (elt state 1))
  (+ 2 (current-column)))

;;(put 'vcase    'scheme-indent-function 'scheme-let-indent)   ;;
;;(put 'match     'scheme-indent-function 'enki-match-indent)  ;;
;;(put 'datatype  'scheme-indent-function 1)                   ;; tagged union (algebraic data types)
;;(put 'map-range 'scheme-indent-function 1)                   ;;
;;(put 'for-range 'scheme-indent-function 1)                   ;;
;;(put 'let/cc    'scheme-indent-function 'scheme-let-indent)  ;;

;; This allows "M-x align" to line up a series of pattern-matches, try it!
;; (add-hook 'align-load-hook (lambda ()
;;                              (add-to-list 'align-rules-list
;;                                           '(enki-patterns
;;                                             (tab-stop . nil)
;;                                             (regexp . "\\(\\s-*\\)\\->\\(\\s-*\\)")
;;                                             (modes . '(enki-mode)))
;;                                           )))

(provide 'enki)
