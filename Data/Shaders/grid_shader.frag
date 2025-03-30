uniform sampler2D sprite_texture;
uniform sampler2D grid_texture;
uniform float texture_width;
uniform float texture_height;
uniform vec4 grid_color;

void main()
{
    vec4 grid_pixel = texture2D( grid_texture, gl_TexCoord[0].xy );
    grid_pixel.rgb = grid_color.rgb;

    if( texture_width <= 0.0 || texture_height <= 0.0 )
    {
        gl_FragColor = grid_pixel;
        return;
    }

    float epsilon_width = 1.0 / texture_width;
    float epsilon_height = 1.0 / texture_height;

    gl_FragColor = vec4( 0.0 );

    vec4 sprite_pixel = texture2D( sprite_texture, gl_TexCoord[0].xy );

    if( sprite_pixel.a <= 0.0 )
        return;

    sprite_pixel = texture2D( sprite_texture, gl_TexCoord[0].xy + vec2( epsilon_width, 0.0 ) );

    if( sprite_pixel.a <= 0.0 )
        return;

    sprite_pixel = texture2D( sprite_texture, gl_TexCoord[0].xy + vec2( - epsilon_width, 0.0 ) );

    if( sprite_pixel.a <= 0.0 )
        return;

    sprite_pixel = texture2D( sprite_texture, gl_TexCoord[0].xy + vec2( 0.0, epsilon_height ) );

    if( sprite_pixel.a <= 0.0 )
        return;

    sprite_pixel = texture2D( sprite_texture, gl_TexCoord[0].xy + vec2( 0.0, - epsilon_height ) );

    if( sprite_pixel.a <= 0.0 )
        return;

    gl_FragColor = grid_pixel;
}