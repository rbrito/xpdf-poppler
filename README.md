Customized version of `xpdf` with the use of the `poppler` library
==================================================================

Motivation
----------


The idea for this project is twofold:

* to fix some of the bugs present in the current xpdf that are not,
  unfortunately, fixed yet (alas, if they had, this would not need to
  exist at all).

* to be conservative, while including a hand-picked amount of features,
  like extended and more fine-grained zooming so that I can actually see
  my PDF files without having too many problems with my eyes.

A more detailed discussion of the things that motivated this initiative
can be seen on [my original post][0].

Organization of the Project
---------------------------


You are welcome to see other places than the `master` branch: if you
want a very basic, semi-broken packaging for Debian, see the [debian][1]
branch. A rough approximation of the content of the original source by
Martin Pitt is present at [original][2].

When I started this project, I did not know that some people from Gentoo
had taken the changes by Martin and continued keeping up with the newer
versions of [poppler][3] and that Michael Gilbert had [picked up those
changes][4] to include in Debian.

As a result, I restarted almost all the work (with some twists) on top
of the pristine xpdf 3.02 sources, and started to strip things down, by
removing everything that wasn't of use anymore. Those sources are
present at [original-plus-xpdf-3.02][5].

It should be noted that the latter 4 patchlevels released by the
original author of xpdf are not relevant to the present program, since
those changes modify things that are *not* here anymore.  OK.  That
covers the basic information about some of the security updates.

Contributions
-------------


The code presents many opportunities for experimentation and learning
with refactoring, since it has very large classes/functions, a lot of
variables in some functions, code that can be replaced with things that
are part of the standard library etc. It also lacks some const
correctness and has more casts than what would be desired.

Any contributions are welcome and will be sincerely considered,
especially if they help to improve the source code readability.



Rogério Brito

----

[0]: http://rb.doesntexist.org/blog/posts/please-let-me-zoom-my-documents-/
[1]: http://github.com/rbrito/xpdf-poppler/tree/debian
[2]: http://github.com/rbrito/xpdf-poppler/tree/original
[3]: http://poppler.freedesktop.org/
[4]: http://git.debian.org/?p=collab-maint/xpdf.git
[5]: http://github.com/rbrito/xpdf-poppler/tree/original-plus-xpdf-3.02
