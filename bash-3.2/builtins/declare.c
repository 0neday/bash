/* declare.c, created from declare.def. */
#line 23 "declare.def"

#line 52 "declare.def"

#line 58 "declare.def"

#include <config.h>

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <stdio.h>

#include "../bashansi.h"
#include "../bashintl.h"

#include "../shell.h"
#include "common.h"
#include "builtext.h"
#include "bashgetopt.h"

extern int array_needs_making;
extern int posixly_correct;

static int declare_internal __P((register WORD_LIST *, int));

/* Declare or change variable attributes. */
int
declare_builtin (list)
     register WORD_LIST *list;
{
  return (declare_internal (list, 0));
}

#line 98 "declare.def"
int
local_builtin (list)
     register WORD_LIST *list;
{
  if (variable_context)
    return (declare_internal (list, 1));
  else
    {
      builtin_error (_("can only be used in a function"));
      return (EXECUTION_FAILURE);
    }
}

#if defined (ARRAY_VARS)
#  define DECLARE_OPTS	"+afiprtxF"
#else
#  define DECLARE_OPTS	"+fiprtxF"
#endif

/* The workhorse function. */
static int
declare_internal (list, local_var)
     register WORD_LIST *list;
     int local_var;
{
  int flags_on, flags_off, *flags, any_failed, assign_error, pflag, nodefs, opt;
  char *t, *subscript_start;
  SHELL_VAR *var;
  FUNCTION_DEF *shell_fn;

  flags_on = flags_off = any_failed = assign_error = pflag = nodefs = 0;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, DECLARE_OPTS)) != EOF)
    {
      flags = list_opttype == '+' ? &flags_off : &flags_on;

      switch (opt)
	{
	case 'a':
#if defined (ARRAY_VARS)
	  *flags |= att_array;
#endif
	  break;
	case 'p':
	  if (local_var == 0)
	    pflag++;
	  break;
        case 'F':
	  nodefs++;
	  *flags |= att_function;
	  break;
	case 'f':
	  *flags |= att_function;
	  break;
	case 'i':
	  *flags |= att_integer;
	  break;
	case 'r':
	  *flags |= att_readonly;
	  break;
	case 't':
	  *flags |= att_trace;
	  break;
	case 'x':
	  *flags |= att_exported;
	  array_needs_making = 1;
	  break;
	default:
	  builtin_usage ();
	  return (EX_USAGE);
	}
    }

  list = loptend;

  /* If there are no more arguments left, then we just want to show
     some variables. */
  if (list == 0)	/* declare -[afFirtx] */
    {
      /* Show local variables defined at this context level if this is
	 the `local' builtin. */
      if (local_var)
	{
	  register SHELL_VAR **vlist;
	  register int i;

	  vlist = all_local_variables ();

	  if (vlist)
	    {
	      for (i = 0; vlist[i]; i++)
		print_assignment (vlist[i]);

	      free (vlist);
	    }
	}
      else
	{
	  if (flags_on == 0)
	    set_builtin ((WORD_LIST *)NULL);
	  else
	    set_or_show_attributes ((WORD_LIST *)NULL, flags_on, nodefs);
	}

      fflush (stdout);
      return (EXECUTION_SUCCESS);
    }

  if (pflag)	/* declare -p [-afFirtx] name [name...] */
    {
      for (any_failed = 0; list; list = list->next)
	{
	  pflag = show_name_attributes (list->word->word, nodefs);
	  if (pflag)
	    {
	      sh_notfound (list->word->word);
	      any_failed++;
	    }
	}
      return (any_failed ? EXECUTION_FAILURE : EXECUTION_SUCCESS);
    }

#define NEXT_VARIABLE() free (name); list = list->next; continue

  /* There are arguments left, so we are making variables. */
  while (list)		/* declare [-afFirx] name [name ...] */
    {
      char *value, *name;
      int offset, aflags;
#if defined (ARRAY_VARS)
      int making_array_special, compound_array_assign, simple_array_assign;
#endif

      name = savestring (list->word->word);
      offset = assignment (name, 0);
      aflags = 0;

      if (offset)	/* declare [-afFirx] name=value */
	{
	  name[offset] = '\0';
	  value = name + offset + 1;
	  if (name[offset - 1] == '+')
	    {
	      aflags |= ASS_APPEND;
	      name[offset - 1] = '\0';
	    }
	}
      else
	value = "";

#if defined (ARRAY_VARS)
      compound_array_assign = simple_array_assign = 0;
      subscript_start = (char *)NULL;
      if (t = strchr (name, '['))	/* ] */
	{
	  subscript_start = t;
	  *t = '\0';
	  making_array_special = 1;
	}
      else
	making_array_special = 0;
#endif

      /* If we're in posix mode or not looking for a shell function (since
	 shell function names don't have to be valid identifiers when the
	 shell's not in posix mode), check whether or not the argument is a
	 valid, well-formed shell identifier. */
      if ((posixly_correct || (flags_on & att_function) == 0) && legal_identifier (name) == 0)
	{
	  sh_invalidid (name);
	  assign_error++;
	  NEXT_VARIABLE ();
	}

      /* If VARIABLE_CONTEXT has a non-zero value, then we are executing
	 inside of a function.  This means we should make local variables,
	 not global ones. */

      /* XXX - this has consequences when we're making a local copy of a
	       variable that was in the temporary environment.  Watch out
	       for this. */
      if (variable_context && ((flags_on & att_function) == 0))
	{
#if defined (ARRAY_VARS)
	  if ((flags_on & att_array) || making_array_special)
	    var = make_local_array_variable (name);
	  else
#endif
	  var = make_local_variable (name);
	  if (var == 0)
	    {
	      any_failed++;
	      NEXT_VARIABLE ();
	    }
	}
      else
	var = (SHELL_VAR *)NULL;

      /* If we are declaring a function, then complain about it in some way.
	 We don't let people make functions by saying `typeset -f foo=bar'. */

      /* There should be a way, however, to let people look at a particular
	 function definition by saying `typeset -f foo'. */

      if (flags_on & att_function)
	{
	  if (offset)	/* declare -f [-rix] foo=bar */
	    {
	      builtin_error (_("cannot use `-f' to make functions"));
	      free (name);
	      return (EXECUTION_FAILURE);
	    }
	  else		/* declare -f [-rx] name [name...] */
	    {
	      var = find_function (name);

	      if (var)
		{
		  if (readonly_p (var) && (flags_off & att_readonly))
		    {
		      builtin_error (_("%s: readonly function"), name);
		      any_failed++;
		      NEXT_VARIABLE ();
		    }

		  /* declare -[Ff] name [name...] */
		  if (flags_on == att_function && flags_off == 0)
		    {
#if defined (DEBUGGER)
		      if (nodefs && debugging_mode)
			{
			  shell_fn = find_function_def (var->name);
			  if (shell_fn)
			    printf ("%s %d %s\n", var->name, shell_fn->line, shell_fn->source_file);
			  else
			    printf ("%s\n", var->name);
			}
		      else
#endif /* DEBUGGER */
			{	
			  t = nodefs ? var->name
				     : named_function_string (name, function_cell (var), 1);
			  printf ("%s\n", t);
			}
		    }
		  else		/* declare -[fF] -[rx] name [name...] */
		    {
		      VSETATTR (var, flags_on);
		      VUNSETATTR (var, flags_off);
		    }
		}
	      else
		any_failed++;
	      NEXT_VARIABLE ();
	    }
	}
      else		/* declare -[airx] name [name...] */
	{
	  /* Non-null if we just created or fetched a local variable. */
	  if (var == 0)
	    var = find_variable (name);

	  if (var == 0)
	    {
#if defined (ARRAY_VARS)
	      if ((flags_on & att_array) || making_array_special)
		var = make_new_array_variable (name);
	      else
#endif
	      var = bind_variable (name, "", 0);
	    }

	  /* Cannot use declare +r to turn off readonly attribute. */ 
	  if (readonly_p (var) && (flags_off & att_readonly))
	    {
	      sh_readonly (name);
	      any_failed++;
	      NEXT_VARIABLE ();
	    }

	  /* Cannot use declare to assign value to readonly or noassign
	     variable. */
	  if ((readonly_p (var) || noassign_p (var)) && offset)
	    {
	      if (readonly_p (var))
		sh_readonly (name);
	      assign_error++;
	      NEXT_VARIABLE ();
	    }

#if defined (ARRAY_VARS)
	  if ((making_array_special || (flags_on & att_array) || array_p (var)) && offset)
	    {
	      int vlen;
	      vlen = STRLEN (value);
#if 0
	      if (value[0] == '(' && strchr (value, ')'))
#else
	      if (value[0] == '(' && value[vlen-1] == ')')
#endif
		compound_array_assign = 1;
	      else
		simple_array_assign = 1;
	    }

	  /* Cannot use declare +a name to remove an array variable. */
	  if ((flags_off & att_array) && array_p (var))
	    {
	      builtin_error (_("%s: cannot destroy array variables in this way"), name);
	      any_failed++;
	      NEXT_VARIABLE ();
	    }

	  /* declare -a name makes name an array variable. */
	  if ((making_array_special || (flags_on & att_array)) && array_p (var) == 0)
	    var = convert_var_to_array (var);
#endif /* ARRAY_VARS */

	  VSETATTR (var, flags_on);
	  VUNSETATTR (var, flags_off);

#if defined (ARRAY_VARS)
	  if (offset && compound_array_assign)
	    assign_array_var_from_string (var, value, aflags);
	  else if (simple_array_assign && subscript_start)
	    {
	      /* declare [-a] name[N]=value */
	      *subscript_start = '[';	/* ] */
	      var = assign_array_element (name, value, 0);	/* XXX - not aflags */
	      *subscript_start = '\0';
	    }
	  else if (simple_array_assign)
	    /* let bind_array_variable take care of this. */
	    bind_array_variable (name, 0, value, aflags);
	  else
#endif
	  /* bind_variable_value duplicates the essential internals of
	     bind_variable() */
	  if (offset)
	    bind_variable_value (var, value, aflags);

	  /* If we found this variable in the temporary environment, as with
	     `var=value declare -x var', make sure it is treated identically
	     to `var=value export var'.  Do the same for `declare -r' and
	     `readonly'.  Preserve the attributes, except for att_tempvar. */
	  /* XXX -- should this create a variable in the global scope, or
	     modify the local variable flags?  ksh93 has it modify the
	     global scope.
	     Need to handle case like in set_var_attribute where a temporary
	     variable is in the same table as the function local vars. */
	  if ((flags_on & (att_exported|att_readonly)) && tempvar_p (var))
	    {
	      SHELL_VAR *tv;
	      char *tvalue;

	      tv = find_tempenv_variable (var->name);
	      if (tv)
		{
		  tvalue = var_isset (var) ? savestring (value_cell (var)) : savestring ("");
	          tv = bind_variable (var->name, tvalue, 0);
	          tv->attributes |= var->attributes & ~att_tempvar;
	          if (tv->context > 0)
		    VSETATTR (tv, att_propagate);
	          free (tvalue);
		}
	      VSETATTR (var, att_propagate);
	    }
	}

      stupidly_hack_special_variables (name);

      NEXT_VARIABLE ();
    }

  return (assign_error ? EX_BADASSIGN
		       : ((any_failed == 0) ? EXECUTION_SUCCESS
  					    : EXECUTION_FAILURE));
}
