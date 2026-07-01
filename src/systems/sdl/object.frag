uniform sampler2D image;
uniform vec4 colour;
uniform float mono;
uniform float invert;
uniform vec3 tint;
uniform float bright;
uniform float dark;
uniform float alpha;

void tinter(in float pixel_val, in float tint_val, out float mixed) {
  if (tint_val > 0.0) {
    mixed = pixel_val + tint_val - (pixel_val * tint_val);
  } else if (tint_val < 0.0) {
    mixed = pixel_val * abs(tint_val);
  } else {
    mixed = pixel_val;
  }
}

void main() {
  vec4 original = texture2D(image, gl_TexCoord[0].st);
  vec4 pixel = original;

  // Apply inversion effect
  if (invert > 0.0) {
    vec3 inverted = vec3(1.0) - original.rgb;
    vec3 mixed = mix(pixel.rgb, inverted, invert);
    pixel.rgb = mixed;
  }

  // Apply grayscale effect
  if (mono > 0.0) {
    float gray = dot(original.rgb, vec3(0.299, 0.587, 0.114));
    vec3 mixed = mix(pixel.rgb, vec3(gray), mono);
    pixel.rgb = mixed;
  }

  pixel.rgb = clamp(pixel.rgb + vec3(bright) - vec3(dark), 0.0, 1.0);
  pixel.a = original.a;

  // The colour is blended directly with the incoming pixel value.
  vec3 coloured = mix(pixel.rgb, colour.rgb, colour.a);
  pixel = vec4(coloured.r, coloured.g, coloured.b, pixel.a);

  float out_r, out_g, out_b;
  tinter(pixel.r, tint.r, out_r);
  tinter(pixel.g, tint.g, out_g);
  tinter(pixel.b, tint.b, out_b);
  pixel = vec4(out_r, out_g, out_b, pixel.a);

  // We're responsible for doing the main alpha blending, too.
  pixel.a = pixel.a * alpha;
  gl_FragColor = pixel;
}
