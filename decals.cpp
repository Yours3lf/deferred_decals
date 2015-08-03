#include "framework.h"

#include "octree.h"
#include "intersection.h"

using namespace prototyper;

int main( int argc, char** argv )
{
  shape::set_up_intersection();

  map<string, string> args;

  for( int c = 1; c < argc; ++c )
  {
    args[argv[c]] = c + 1 < argc ? argv[c + 1] : "";
    ++c;
  }

  cout << "Arguments: " << endl;
  for_each( args.begin(), args.end(), []( pair<string, string> p )
  {
    cout << p.first << " " << p.second << endl;
  } );

  uvec2 screen( 0 );
  bool fullscreen = false;
  bool silent = false;
  string title = "Basic sponza CPP to get started";

  /*
   * Process program arguments
   */

  stringstream ss;
  ss.str( args["--screenx"] );
  ss >> screen.x;
  ss.clear();
  ss.str( args["--screeny"] );
  ss >> screen.y;
  ss.clear();

  if( screen.x == 0 )
  {
    screen.x = 1280;
  }

  if( screen.y == 0 )
  {
    screen.y = 720;
  }

  try
  {
    args.at( "--fullscreen" );
    fullscreen = true;
  }
  catch( ... ) {}

  try
  {
    args.at( "--help" );
    cout << title << ", written by Marton Tamas." << endl <<
         "Usage: --silent      //don't display FPS info in the terminal" << endl <<
         "       --screenx num //set screen width (default:1280)" << endl <<
         "       --screeny num //set screen height (default:720)" << endl <<
         "       --fullscreen  //set fullscreen, windowed by default" << endl <<
         "       --help        //display this information" << endl;
    return 0;
  }
  catch( ... ) {}

  try
  {
    args.at( "--silent" );
    silent = true;
  }
  catch( ... ) {}

  /*
   * Initialize the OpenGL context
   */

  framework frm;
  frm.init( screen, title, fullscreen );
  frm.set_vsync( true );

  //set opengl settings
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LEQUAL );
  glFrontFace( GL_CCW );
  glEnable( GL_CULL_FACE );
  glClearColor( 0.5f, 0.5f, 0.8f, 0.0f ); //sky color
  glClearDepth( 1.0f );

  frm.get_opengl_error();

  /*
   * Set up mymath
   */

  camera<float> cam;
  frame<float> the_frame;

  float cam_fov = 45.0f;
  float cam_near = 2.5f;
  float cam_far = 2500.0f;

  the_frame.set_perspective( radians( cam_fov ), ( float )screen.x / ( float )screen.y, cam_near, cam_far );

  cam.move_forward( -5 );
  cam.move_up( 1 );

  glViewport( 0, 0, screen.x, screen.y );

  /*
   * Set up the scene
   */

  octree<unsigned>* o = new octree<unsigned>(aabb(vec3(0), vec3(512)));
  o->set_up_octree(&o);

  GLuint quad = frm.create_quad( the_frame.far_ll.xyz, the_frame.far_lr.xyz, the_frame.far_ul.xyz, the_frame.far_ur.xyz );

  float move_amount = 10;

  int minmeshes = 0;
  int maxmeshes = 397;

  scene s;
  /*mesh::load_into_meshes( app_path + "resources/sponza_dae/sponza.dae", s, true );

  unsigned counter2 = 0;
  for( auto& c : s.meshes )
  {
  ss.clear();
  ss.str( "" );
  ss << "../resources/mesh/mesh" << counter2++ << ".mesh";
  c.write_mesh( ss.str() );
  }*/

  //load meshes
  s.meshes.resize( maxmeshes - minmeshes );

  for( int c = minmeshes; c < maxmeshes; ++c )
  {
    stringstream ss;
    ss << "../resources/mesh/mesh" << c << ".mesh";
    int idx = c - minmeshes;
    s.meshes[idx].read_mesh( ss.str( ) );
  }

  //create aabb and insert into octree
  int counter = 0;
  for( auto& c : s.meshes )
  {
    c.local_bv = new aabb( );

    for( int i = 0; i < c.vertices.size( ); i += 3 )
    {
      static_cast<aabb*>( c.local_bv )->expand( vec3(
        c.vertices[i + 0],
        c.vertices[i + 1],
        c.vertices[i + 2]
        ) );
    }

    o->insert( counter++, c.local_bv );
  }

  for( auto & c : s.meshes )
    c.upload( );

  //cleanup
  for( auto& c : s.meshes )
  {
    c.indices.resize( 0 );
    c.indices.reserve( 0 );
    c.vertices.resize( 0 );
    c.vertices.reserve( 0 );
    c.normals.resize( 0 );
    c.normals.reserve( 0 );
    c.tex_coords.resize( 0 );
    c.tex_coords.reserve( 0 );
    c.tangents.resize( 0 );
    c.tangents.reserve( 0 );
  }

  camera<float> light_cam;
  light_cam.rotate_y( radians( 65.0f ) );
  light_cam.rotate_x( radians( -65.0f ) );

  GLuint smiley = frm.load_image( "../resources/decals/smiley.png" );
  GLuint smiley_nrm = frm.load_image( "../resources/decals/smiley_NRM.png" );

  scene box;
  mesh::load_into_meshes( "../resources/decals/smiley.obj", box );

  /*
   * Set up the shaders
   */

  GLuint lighting_shader = 0;
  frm.load_shader( lighting_shader, GL_VERTEX_SHADER, "../shaders/decals/lighting.vs" );
  frm.load_shader( lighting_shader, GL_FRAGMENT_SHADER, "../shaders/decals/lighting.ps" );

  GLuint gbuffer_shader = 0;
  frm.load_shader( gbuffer_shader, GL_VERTEX_SHADER, "../shaders/decals/gbuffer.vs" );
  frm.load_shader( gbuffer_shader, GL_FRAGMENT_SHADER, "../shaders/decals/gbuffer.ps" );

  GLuint decal_shader = 0;
  frm.load_shader( decal_shader, GL_VERTEX_SHADER, "../shaders/decals/decal.vs" );
  frm.load_shader( decal_shader, GL_FRAGMENT_SHADER, "../shaders/decals/decal.ps" );

  GLint lighting_mv_mat_loc = glGetUniformLocation( lighting_shader, "mv" );
  GLint lighting_mvp_mat_loc = glGetUniformLocation( lighting_shader, "mvp" );
  GLint lighting_dir_loc = glGetUniformLocation( lighting_shader, "light_dir" );

  GLint gbuffer_mvp_mat_loc = glGetUniformLocation( gbuffer_shader, "mvp" );
  GLint gbuffer_normal_mat_loc = glGetUniformLocation( gbuffer_shader, "normal_mat" );

  GLint decal_mvp_mat_loc = glGetUniformLocation( decal_shader, "mvp" );
  GLint decal_normal_mat_loc = glGetUniformLocation( decal_shader, "normal_mat" );
  GLint decal_inv_mv_mat_loc = glGetUniformLocation( decal_shader, "inv_mv" );
  GLint decal_mv_mat_loc = glGetUniformLocation( decal_shader, "mv" );
  GLint decal_ll_loc = glGetUniformLocation( decal_shader, "ll" );
  GLint decal_ur_loc = glGetUniformLocation( decal_shader, "ur" );
  GLint decal_cam_pos_loc = glGetUniformLocation( decal_shader, "cam_pos" );

  /*
   * Set up fbos / textures
   */

  //set up deferred shading
  GLuint depth_texture = 0;
  glGenTextures( 1, &depth_texture );

  glBindTexture( GL_TEXTURE_2D, depth_texture );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, screen.x, screen.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0 );

  GLuint normal_texture = 0;
  glGenTextures( 1, &normal_texture );

  glBindTexture( GL_TEXTURE_2D, normal_texture );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, screen.x, screen.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );

  GLuint albedo_texture = 0;
  glGenTextures( 1, &albedo_texture );

  glBindTexture( GL_TEXTURE_2D, albedo_texture );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, screen.x, screen.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );

  //set up deferred shading
  GLuint gbuffer_fbo = 0;
  glGenFramebuffers( 1, &gbuffer_fbo );
  glBindFramebuffer( GL_FRAMEBUFFER, gbuffer_fbo );
  GLenum buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers( 2, buffers );

  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, albedo_texture, 0 );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normal_texture, 0 );
  glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0 );

  frm.check_fbo_status();

  /*
   * Handle events
   */

  bool warped = false, ignore = true;
  vec2 movement_speed = vec2(0);

  auto event_handler = [&]( const sf::Event & ev )
  {
    switch( ev.type )
    {
      case sf::Event::MouseMoved:
        {
          vec2 mpos( ev.mouseMove.x / float( screen.x ), ev.mouseMove.y / float( screen.y ) );

          if( warped )
          {
            ignore = false;
          }
          else
          {
            frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
            warped = true;
            ignore = true;
          }

          if( !ignore && all( notEqual( mpos, vec2( 0.5 ) ) ) )
          {
            cam.rotate( radians( -180.0f * ( mpos.x - 0.5f ) ), vec3( 0.0f, 1.0f, 0.0f ) );
            cam.rotate_x( radians( -180.0f * ( mpos.y - 0.5f ) ) );
            frm.set_mouse_pos( ivec2( screen.x / 2.0f, screen.y / 2.0f ) );
            warped = true;
          }
        }
      case sf::Event::KeyPressed:
        {
          /*if( ev.key.code == sf::Keyboard::A )
          {
            cam.rotate_y( radians( cam_rotation_amount ) );
          }*/
        }
      default:
        break;
    }
  };

  /*
   * Render
   */

  sf::Clock timer;
  timer.restart();

  sf::Clock movement_timer;
  movement_timer.restart();

  float orig_mov_amount = move_amount;

  frm.display( [&]
  {
    frm.handle_events( event_handler );

    float seconds = movement_timer.getElapsedTime().asMilliseconds() / 1000.0f;

    if( sf::Keyboard::isKeyPressed( sf::Keyboard::LShift ) || sf::Keyboard::isKeyPressed( sf::Keyboard::RShift ) )
    {
      move_amount = orig_mov_amount * 3.0f;
    }
    else
    {
      move_amount = orig_mov_amount;
    }

    if( seconds > 0.01667 )
    {
      //move camera
      if( sf::Keyboard::isKeyPressed( sf::Keyboard::A ) )
      {
        movement_speed.x -= move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::D ) )
      {
        movement_speed.x += move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::W ) )
      {
        movement_speed.y += move_amount;
      }

      if( sf::Keyboard::isKeyPressed( sf::Keyboard::S ) )
      {
        movement_speed.y -= move_amount;
      }

      cam.move_forward( movement_speed.y * seconds );
      cam.move_right( movement_speed.x * seconds );
      movement_speed *= 0.5;

      movement_timer.restart();
    }

    //-----------------------------
    //set up matrices
    //-----------------------------

    mat4 model = create_translation( -cam.pos );
    mat4 view = cam.get_matrix();
    mat4 proj = the_frame.projection_matrix;
    mat4 mv = view * model;
    mat4 inv_mv = inverse( mv );
    mat4 normal_mat = mv;
    vec4 vs_eye_pos = mv * vec4( cam.pos, 1 );
    mat4 mvp = proj * view * model;

    frustum f;
    f.set_up( cam, the_frame );

    //cull objects
    static vector<unsigned> culled_objs;
    culled_objs.clear();
    o->get_culled_objects( culled_objs, &f );

    //-----------------------------
    //gbuffer rendering
    //-----------------------------

    glViewport( 0, 0, screen.x, screen.y );

    glEnable( GL_DEPTH_TEST );

    glBindFramebuffer( GL_FRAMEBUFFER, gbuffer_fbo );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram( gbuffer_shader );

    glUniformMatrix4fv( gbuffer_mvp_mat_loc, 1, false, &mvp[0][0] );
    glUniformMatrix4fv( gbuffer_normal_mat_loc, 1, false, &normal_mat[0][0] );

    for( auto& c : culled_objs )
      s.meshes[c].render();

    glDisable( GL_DEPTH_TEST );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    //-----------------------------
    //decal rendering
    //-----------------------------

    /**/

    glBindFramebuffer( GL_FRAMEBUFFER, gbuffer_fbo );

    glUseProgram( decal_shader );

    glEnable( GL_DEPTH_TEST );
    glDepthMask( false );
    glEnable( GL_BLEND );

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glUniformMatrix4fv( decal_mvp_mat_loc, 1, false, &mvp[0][0] );
    glUniformMatrix4fv( decal_normal_mat_loc, 1, false, &normal_mat[0][0] );
    glUniformMatrix4fv( decal_inv_mv_mat_loc, 1, false, &inv_mv[0][0] );
    glUniformMatrix4fv( decal_mv_mat_loc, 1, false, &mv[0][0] );
    glUniform3fv( decal_cam_pos_loc, 1, &cam.pos.x );
    glUniform3fv( decal_ll_loc, 1, &the_frame.far_ll.x );
    glUniform3fv( decal_ur_loc, 1, &the_frame.far_ur.x );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, depth_texture );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, smiley );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, smiley_nrm );
    glActiveTexture( GL_TEXTURE0 );

    box.meshes[0].render();

    glDepthMask( true );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    /**/

    //-----------------------------
    //render the lights
    //-----------------------------

    /**/

    vec4 vs_light_dir = mv * vec4(-light_cam.view_dir, 0);

    //lighting goes here
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    glViewport( 0, 0, screen.x, screen.y );

    glUseProgram( lighting_shader );

    glUniform3fv( lighting_dir_loc, 1, &vs_light_dir.x );

    glUniformMatrix4fv( lighting_mvp_mat_loc, 1, false, &proj[0][0] );
    glUniformMatrix4fv( lighting_mv_mat_loc, 1, false, &mat4::identity[0][0] );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, albedo_texture );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, normal_texture );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D, depth_texture );
    glActiveTexture( GL_TEXTURE0 );

    glBindVertexArray( quad );
    glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0 );

    /**/

    frm.get_opengl_error();
  }, silent );

  return 0;
}
