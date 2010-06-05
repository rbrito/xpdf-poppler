Customized version of xpdf with the use of the poppler library
--------------------------------------------------------------

The idea for this is twofold:

* to fix some of the bugs present in the current xpdf that are not,
  unfortunately, fixed yet (alas, if they had, this would not need to
  exist at all).

* to include more zooming so that I can actually see my PDF files
  without having too many problems with my eyes.

A more detailed discussion of the things that motivated this initiative
can be seen on [my original post][0].

You are welcome to see other places than the `master` branch: if you
want a very basic, semi-broken packaging for Debian, see the [debian][1]
branch. A rough approximation of the content of the original source by
Martin Pitt is present at [original][2].

When I started this project, I did not know that some people from Gentoo
had taken the changes by Martin and continued keeping up with the newer
versions of poppler and that Michael Gilbert had picked those changes to
include in Debian.

As a result, I restarted almost all the work (with some twists) on top
of the pristine xpdf 3.02 sources, and started to strip things down, by
removing everything that wasn't of use anymore. Those sources are
present in the beginning of the [xpdf-3.02 branch][3].

It should be noted that the latter 4 patchlevels released by the
original author of xpdf are not relevant to the present program, since
those changes modify things that are not here anymore.  OK. That covers
the basic information about some of the security updates.

Any contributions are welcome and will be sincerely considered,
especially if they help to improve the source code readability.


Rogério Brito

[0]: http://rb.doesntexist.org/blog/2010/05/27/please-let-me-zoom-my-documents/
[1]: http://github.com/rbrito/xpdf-poppler/tree/debian
[2]: http://github.com/rbrito/xpdf-poppler/tree/original
[3]: http://github.com/rbrito/xpdf-poppler/tree/xpdf-3.02
