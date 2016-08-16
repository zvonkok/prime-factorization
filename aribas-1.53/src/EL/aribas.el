;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; aribas.el
;; Elisp code for running ARIBAS from within GNU Emacs (v. 19.xx or higher)
;; 1996-02-20, Otto Forster
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; To be able to run ARIBAS from within Emacs, this file must
;; be in the load-path of Emacs. If this is not the case, you
;; can extend the load-path by customizing your .emacs file.
;; If for example aribas.el is in the directory /usr/local/lib/aribas,
;; then write the following Elisp command into .emacs
;;
;; (setq load-path (cons "/usr/local/lib/aribas" load-path))
;;
;; If aribas.el is in the subdirectory el of your home directory,
;; you can write
;;
;; (setq load-path (cons (expand-file-name "~/el") load-path))
;;
;; Furthermore, put the following lines into your .emacs file
;;
;; (autoload 'run-aribas "aribas"
;;      "Run ARIBAS." t)
;;
;; Then, after the next start of Emacs, you can run ARIBAS
;; by giving the command
;;
;;   M-x run-aribas
;;
;; (If you don't have a META key, use ESC x instead of M-x)
;; Then ARIBAS will be loaded into an Emacs window with name 
;; *aribas* and you can edit your input to ARIBAS.
;; If your input ends with a full stop '.' and you press RETURN,
;; it is sent to ARIBAS.
;; If however your complete input does not end with a full stop,
;; (for example in response to a readln), the input is sent 
;; to ARIBAS by C-j (Control-j) or C-c RETURN.
;; If you want to repeat a previous input, M-p cycles backward
;; through input history, and M-n cycles forward.
;; A Control-C is sent to ARIBAS by C-c C-c (press C-c twice).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(require 'comint)

(defvar aribas-mode-hook nil
  "*Hook for customising aribas-mode.")

(defvar aribas-input-prompt "^==>[ \t]*")

(defun aribas-input-complete ()
  ;; check if point is at input end       
  (let* ((res nil)
	 (savepoint (point))
	 (str1 (buffer-substring savepoint (progn (end-of-line) (point))))
	 (str2 (buffer-substring (progn (beginning-of-line) (point)) 
				 savepoint)))
    (cond ((not (string-match "^[ \t]*$" str1)))
	  ((or (string-match ".*\\.[ \t]*$" str2) 
	       (string-match ".*\\?[ \t]*$" str2) 
	       (string-match "exit[ \t]*$" str2))
	   (setq res t)))
	  (goto-char savepoint)
	  res))

(defun aribas-newline-or-send-input ()
  (interactive)
  (if (aribas-input-complete)
      (comint-send-input)
    (newline)))

(defun aribas-tab ()
  "Indent to next tab stop."
  (interactive)
  (indent-to (* (1+ (/ (current-indentation) aribas-indent)) aribas-indent)))

(defvar aribas-indent 4 
  "*This variable gives the indentation in aribas-mode")

(defun aribas-mode-commands (map)
  (define-key map "\C-m" 'aribas-newline-or-send-input)
  (define-key map "\C-j" 'comint-send-input)
  (define-key map "\C-c\C-m" 'comint-send-input)
  (define-key map "\C-i" 'aribas-tab))

(defvar aribas-mode-map nil)

(if aribas-mode-map
    nil
  ;; if aribas-mode-map has already been set, do nothing,
  ;; else do the following
  (setq aribas-mode-map (copy-keymap comint-mode-map))
  (aribas-mode-commands aribas-mode-map))

(defvar aribas-mode-syntax-table nil
  "Syntax table in use in aribas-mode buffers.")

(if aribas-mode-syntax-table
    nil
  (let ((table (make-syntax-table)))
    (modify-syntax-entry ?\( ". 1" table)
    (modify-syntax-entry ?\) ". 4" table)
    (modify-syntax-entry ?* ". 23" table)
    (modify-syntax-entry ?+ "." table)
    (modify-syntax-entry ?- "." table)
    (modify-syntax-entry ?= "." table)
    (modify-syntax-entry ?% "." table)
    (modify-syntax-entry ?< "." table)
    (modify-syntax-entry ?> "." table)
    (modify-syntax-entry ?_ "w" table)
    (modify-syntax-entry ?\\ "_" table)
    (modify-syntax-entry ?! "_" table)
    (modify-syntax-entry ?? "_" table)
    (modify-syntax-entry ?$ "'" table)
    (modify-syntax-entry ?& "'" table)
    (modify-syntax-entry ?\' "/" table)
    (setq aribas-mode-syntax-table table)))

(defun aribas-mode ()
  "Major mode for interacting with Aribas process.

An Aribas process can be fired up with M-x run-aribas.

Customisation: Entry to this mode runs the hooks on comint-mode-hook and
aribas-mode-hook (in that order).

Commands:
RETURN after the end of an input ending with a full stop `.'
sends the text from the aribas prompt to point to ARIBAS.
If the input does not end with a full stop, RETURN produces
a newline.
CTRL-J  sends the text from the aribas prompt to point to ARIBAS.
CTRL-C CTRL-C sends a Control-C to ARIBAS.
META-P cycles backwards in input history,
META-N cycles forward."
  (interactive)
  (comint-mode)
  ;; Customise in aribas-mode-hook
  (setq comint-prompt-regexp aribas-input-prompt)
  (aribas-mode-variables)
  (setq major-mode 'aribas-mode)
  (setq mode-name "Aribas")
  (setq mode-line-process '(": %s"))
  (set-syntax-table aribas-mode-syntax-table)
  (use-local-map aribas-mode-map)
  (setq comint-scroll-to-bottom-on-output 'this)
  (setq comint-scroll-show-maximum-output t)
  (setq comint-input-autoexpand nil)
  (setq comint-input-filter (function aribas-input-filter))
  (setq comint-input-sentinel (function ignore))
  (setq comint-get-old-input (function aribas-get-old-input))
  (setq case-fold-search nil)
  (run-hooks 'aribas-mode-hook))

(defun aribas-input-filter (str)
  t)

(defun aribas-mode-variables ()
  )

(defun aribas-args-to-list (string)
  (let ((where (string-match "[ \t]" string)))
    (cond ((null where) (list string))
	  ((not (= where 0))
	   (cons (substring string 0 where)
		 (aribas-args-to-list (substring string (+ 1 where)
						 (length string)))))
	  (t (let ((pos (string-match "[^ \t]" string)))
	       (if (null pos)
		   nil
		 (aribas-args-to-list (substring string pos
						 (length string)))))))))

(defvar aribas-program-name "aribas"
  "*Program invoked by the run-aribas command")

(defun run-aribas (cmd)
  "Run aribas process, input and output via buffer *aribas*.
If there is a process already running in *aribas*, just switch to that 
buffer. With argument, allows you to edit the command line (default is value
of aribas-program-name).  Runs the hooks from aribas-mode-hook
\(after the comint-mode-hook is run).
\(Type \\[describe-mode] in the process buffer for a list of commands.)"

  (interactive (list (if current-prefix-arg
                         (read-string "Run aribas: " aribas-program-name)
                         aribas-program-name)))
  (if (not (comint-check-proc "*aribas*"))
      (let ((cmdlist (aribas-args-to-list cmd)))
        (set-buffer (apply 'make-comint "aribas" (car cmdlist)
			   nil (cdr cmdlist)))
	(aribas-mode)))
  (setq aribas-buffer "*aribas*")
  (switch-to-buffer "*aribas*"))

(defun aribas-get-old-input ()
  (save-excursion
    (let ((mark (point)))
      (re-search-backward comint-prompt-regexp (point-min) t)
      (comint-skip-prompt)
      (buffer-substring (point) mark))))

(provide 'aribas)

;;; aribas.el ends here
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
