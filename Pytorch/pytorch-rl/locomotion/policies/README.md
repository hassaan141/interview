# Policies

Put the PyTorch actor and critic here once you move beyond the minimal
boilerplate.

Recommended first policy:

- MLP actor with `tanh` hidden activations.
- Gaussian continuous action distribution.
- Learned `log_std` parameter.
- MLP critic with one scalar value output.

Keep the inference path exportable without simulator imports.

