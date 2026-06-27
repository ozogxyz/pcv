# CV / NN from scratch in C — notes

Rawdog the whole stack in pure C (no torch/C++/Python in the hot path). Goal: own every
bit. North star = Karpathy's **llm.c** (GPT-2 train in C→CUDA, no torch) — read as a *map*,
don't copy; skip it for now (big + NLP). Competition work stays Python/torch; this is
the "remember it by building it" track.

## Roadmap / ladder

1. **Image transforms** (flat HWC uint8 buffer, explicit index math) — *in progress*
   - `data[(r*w + c)*n + k]` = channel k of pixel (r,c). Strides `(w*n, n, 1)`.
   - done: hflip (`c → w-1-c`), vflip (`r → h-1-r`), rotate180 (both).
   - stb's flip = same math but swaps whole **rows** via memcpy, in place, h/2 iters.
   - next: **resize** (960×540 → 224) — nearest-neighbor first (`src = dst*scale`), bilinear later.
2. **MLP forward** — Tsoding XOR net (2-2-1). `h = σ(W1·x + b1); ŷ = σ(W2·h + b2)`.
   Per-scalar weights ARE the matrix entries; `W·x` = stack each neuron's weighted sum.
3. **Backprop** (the 4 equations — recurrence, applied per layer):
   ```
   δᴸ     = ∇_ŷ L ⊙ σ'(zᴸ)              (output)
   δˡ     = (Wˡ⁺¹ᵀ · δˡ⁺¹) ⊙ σ'(zˡ)     (push error back one layer)
   ∂L/∂Wˡ = δˡ · (aˡ⁻¹)ᵀ                (outer product: error × layer input)
   ∂L/∂bˡ = δˡ
   ```
   - **Gradient check** day one: `dL/dw ≈ (L(w+ε) − L(w−ε)) / 2ε`. Catches a wrong backward instantly.
4. **Autograd** (optional) — micrograd-style tape, or stay explicit (llm.c style, recommended).
5. **Convolution** — `conv = im2col + matmul`. Once matmul fwd/bwd works, ~90% done.
   - output size: `out = floor((W + 2p − k)/s) + 1`. (540, k3 p1 s1 → 540; s2 → 270.)
   - the conv-specific work = sliding-window index arithmetic + im2col unfold.
6. **CUDA** — only after the CPU version trains. The `(r*w+c)*n+k` index math IS the kernel body.

## Resources (no Medium/Kaggle noise)

Conv nets:
- **CS231n notes** — cs231n.github.io/convolutional-networks/  (the layer + im2col; ~your lost notes)
- **Conv arithmetic** — Dumoulin & Visin, arXiv 1603.07285  (output sizes/stride/pad/dilation + animations)
- **Deep Learning book, Ch.9** — deeplearningbook.org/contents/convnets.html  (free, rigorous, math-first)
- **darknet** (Redmon) source — conv layers in pure C w/ im2col (read to check yourself)

Classical CV (filtering, edges, features, geometry):
- **Szeliski, Computer Vision: Algorithms and Applications** — free PDF at szeliski.org/Book

Lineage / precedent:
- **llm.c** (Karpathy) — pure C→CUDA transformer training, no torch
- **micrograd** (Karpathy) — ~150-line autograd
