" Vim syntax file
" Language:	generic configure file
" Maintainer:	Tim Rice <cryptarch@github>
" Last Change:	2018 Feb 16

" Quit when a (custom) syntax file was already loaded
if exists('b:current_syntax')
  finish
endif

syn match	diceComment	"^#.*"
syn match	diceComment	"\s#.*"ms=s+1
syn match	diceNumber	"\d"
syn match	diceOperator	"x\|d\|+\|-"
syn match       diceDelim       ";"
syn keyword     diceCommand	quit
syn keyword     diceCommand	clear


" Define the default highlighting.
" Only used when an item doesn't have highlighting yet
hi def link diceComment     Comment
hi def link diceNumber      Number
hi def link diceOperator    Statement
hi def link diceDelim       Statement
hi def link diceCommand     Operator

let b:current_syntax = 'dice'

" vim: ts=8 sw=2
