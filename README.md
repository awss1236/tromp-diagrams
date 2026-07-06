# Bigger Description
Found out about different visualizations for lambda calculus expressions recently
and particularly enjoyed the look of [tromp diagrams](https://tromp.github.io/cl/diagrams.html),
so I decided to try to make a small diagram generator in c.

Also does the parsing of lambda expressions from strings, parser implementation inspired from [mpu/lambda](https://github.com/mpu/lambda/).

Also does "greedy" beta-reduction on button press (greedy as in it evaluates at a single step the most shallow, left-most,
beta-reducable expression it can find).
## Todos
The project core is pretty much complete, but I may add some other features to make it easier
to use/for the hell of it.

Maybe add an animation for beta-reduction.
