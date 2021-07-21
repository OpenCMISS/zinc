#include "opencmiss/zinc/zincconfigure.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/mystring.h"
#include "graphics/shader_program.hpp"
#include "graphics/graphics_library.h"
#include "graphics/texture.h"
#include "three_d_drawing/abstract_graphics_buffer.h"
#include "graphics/render_gl.h"
#include "general/compare.h"
#include "graphics/shader_uniforms.hpp"
#include "general/debug.h"
#include "general/indexed_list_private.h"
#include "general/indexed_list_stl_private.hpp"

const char *defaultVertex = "varying vec3 lightPos, eyeVertex, eyeNormal;\n"
							"varying vec4 diffuse;\n"
							"varying float qAttenuation, cAttenuation, lAttenuation;\n"
							"\n"
							"void main()\n"
							"{\n"
							"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
							"    eyeVertex = vec3(gl_ModelViewMatrix * gl_Vertex);\n"
							"    lightPos = gl_LightSource[0].position.xyz ;\n"
							"    eyeNormal = vec3(gl_NormalMatrix * gl_Normal);\n"
							"    diffuse = gl_Color;\n"
							"    qAttenuation = gl_LightSource[0].quadraticAttenuation;\n"
							"    cAttenuation = gl_LightSource[0].constantAttenuation;\n"
							"    lAttenuation = gl_LightSource[0].linearAttenuation;\n"
							"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
							"}\n";

const char *defaultFragment =   "uniform int shadingMode;\n"
								"uniform sampler2D texture0;\n"
								"varying vec3 lightPos, eyeVertex, eyeNormal;\n"
								"varying vec4 diffuse;\n"
								"varying float qAttenuation, cAttenuation, lAttenuation;\n"
								"\n"
								"// calculate texture color based on the texture coordinates\n"
								"vec4 showTexture() {\n"
								"    return texture2D(texture0, vec2(gl_TexCoord[0]));\n"
								"}\n"
								"// per pixel lighting\n"
								"vec3 lightingColor(vec3 inputColor) {\n"
								"    float Len = length(lightPos - eyeVertex);\n"
								"    vec3 lightVector = normalize(lightPos - eyeVertex);\n"
								"    float NdotH = max(dot(eyeNormal, lightVector), 0.1);\n"
								"    float attenuation = 1.0 / (qAttenuation * Len * Len + cAttenuation + Len * lAttenuation);\n"
								"    return inputColor * attenuation * NdotH;\n"
								"}\n"
								"// main loop, calculate the shading differently depending on the mode\n"
								"void main() {\n"
								"    vec4 finalColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
								"    if (shadingMode == 1)\n"
								"    {\n"
								"        finalColor = showTexture();\n"
								"    }\n"
								"    else\n"
								"    {\n"
								"        finalColor = diffuse;\n"
								"    }\n"
								"    gl_FragColor = vec4(lightingColor(vec3(finalColor)), 1.0);\n"
								"}\n";


/*
Module types
------------
*/

struct cmzn_shaderprogram
{
public:
	/*! Specifies the type of the Material Program
	 * These should be unique for each differing program as the materials
	 * will only generate one program for each value of type.
	 * As a special case, type == 0, specifies a predefined arbitrary string
	 * is used and so these should never be shared. */
	enum cmzn_shaderprogram_type type;
#if defined (OPENGL_API)
#if defined GL_ARB_vertex_program && defined GL_ARB_fragment_program
	GLuint vertex_program;
	GLuint fragment_program;
	GLuint geometry_program;
#endif /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
	GLuint glsl_current_program;
	char *vertex_program_string;
	char *geometry_program_string;
	char *fragment_program_string;
	enum cmzn_shaderprogram_shader_type shader_type;
	/*! Display list which enables the correct state for this program */
	GLuint display_list;
#endif /* defined (OPENGL_API) */
	/*! Flag indicating whether the program is compiled or not */
	int compiled;

	const char *name;
	struct MANAGER(cmzn_shaderprogram) *manager;
	cmzn_shaderprogram_change_detail changeDetail;
	bool is_managed_flag;
	int manager_change_status;
	int access_count;


	cmzn_shaderprogram() :
		type(SHADER_PROGRAM_SPECIFIED_STRINGS),
#if defined (OPENGL_API)
#if defined GL_ARB_vertex_program && defined GL_ARB_fragment_program
		vertex_program(0),
		fragment_program(0),
		geometry_program(0),
#endif /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
		glsl_current_program(0),
		vertex_program_string(duplicate_string(defaultVertex)),
		geometry_program_string(0),
		fragment_program_string(duplicate_string(defaultFragment)),
		shader_type(SHADER_PROGRAM_SHADER_NONE),
	/*! Display list which enables the correct state for this program */
		display_list(0),
#endif /* defined (OPENGL_API) */
		compiled(0),
		name(NULL),
		manager(NULL),
		is_managed_flag(false),
		manager_change_status(MANAGER_CHANGE_NONE(cmzn_shaderprogram)),
		access_count(1)
	{
	}

	~cmzn_shaderprogram()
	{
		if (name)
		{
			DEALLOCATE(name);
		}
	#if defined (OPENGL_API)
		if (shader_type==SHADER_PROGRAM_SHADER_GLSL)
		{
			glUseProgram(0);
			if (vertex_program)
			{
				glDeleteShader(vertex_program);
			}
			if (fragment_program)
			{
				glDeleteShader(fragment_program);
			}
			if (geometry_program)
			{
				glDeleteShader(geometry_program);
			}
			if (glsl_current_program)
			{
				glDeleteProgram(glsl_current_program);
			}
		}
		else if (shader_type==SHADER_PROGRAM_SHADER_ARB)
		{
			if (vertex_program)
			{
				glDeleteProgramsARB(1, &vertex_program);
			}
			if (fragment_program)
			{
				glDeleteProgramsARB(1, &fragment_program);
			}
		}
		if (display_list)
		{
			glDeleteLists(display_list, 1);
		}
		if (vertex_program_string)
		{
			DEALLOCATE(vertex_program_string);
			vertex_program_string = NULL;
		}
		if (geometry_program_string)
		{
			DEALLOCATE(geometry_program_string);
			geometry_program_string = NULL;
		}
		if (fragment_program_string)
		{
			DEALLOCATE(fragment_program_string);
			fragment_program_string = NULL;
		}
	#endif /* defined (OPENGL_API) */
	}

	/** must construct on the heap with this function */
	static cmzn_shaderprogram_id create()
	{
		return new cmzn_shaderprogram();
	}

	cmzn_shaderprogram_id access()
	{
		++(this->access_count);
		return this;
	}

	static inline int deaccess(cmzn_shaderprogram_id *object_address)
	{
		cmzn_shaderprogram_id program;

		if (object_address && (program = *object_address))
		{
			--(program->access_count);
			if (program->access_count <= 0)
			{
				delete program;
			}
			program = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

	void programChanged()
	{
		this->compiled = 0;
		this->changeDetail.setProgramChanged();
		MANAGED_OBJECT_CHANGE(cmzn_shaderprogram)(this,
			MANAGER_CHANGE_OBJECT_NOT_IDENTIFIER(cmzn_shaderprogram));
	}

	/** clones and clears type-specific change detail, if any.
	 * override for classes with type-specific change detail
	 * @return  change detail prior to clearing, or NULL if none.
	 */
	cmzn_shaderprogram_change_detail *extractChangeDetail()
	{
		cmzn_shaderprogram_change_detail *change_detail = new cmzn_shaderprogram_change_detail(this->changeDetail);
		this->changeDetail.clear();
		return change_detail;
	}

}; /* cmzn_shaderprogram_id */

/* Only to be used from FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL function
 * Creates a pseudo object with name identifier suitable for finding
 * objects by identifier with cmzn_set.
 */
class cmzn_shaderprogram_identifier : private cmzn_shaderprogram
{
public:
	cmzn_shaderprogram_identifier(const char *name)
	{
		cmzn_shaderprogram::name = name;
	}

	~cmzn_shaderprogram_identifier()
	{
		cmzn_shaderprogram::name = NULL;
	}

	cmzn_shaderprogram_id getPseudoObject()
	{
		return this;
	}
};

/** functor for ordering cmzn_set<cmzn_shaderprogram> by name */
struct cmzn_shaderprogram_compare_name
{
	bool operator() (const cmzn_shaderprogram* program1, const cmzn_shaderprogram* program2) const
	{
		return strcmp(program1->name, program2->name) < 0;
	}
};

typedef cmzn_set<cmzn_shaderprogram_id ,cmzn_shaderprogram_compare_name> cmzn_set_cmzn_shaderprogram;

FULL_DECLARE_MANAGER_TYPE_WITH_OWNER(cmzn_shaderprogram, cmzn_shadermodule, class cmzn_shaderprogram_change_detail *);

DECLARE_DEFAULT_MANAGER_UPDATE_DEPENDENCIES_FUNCTION(cmzn_shaderprogram);

inline cmzn_shaderprogram_change_detail *MANAGER_EXTRACT_CHANGE_DETAIL(cmzn_shaderprogram)(
		cmzn_shaderprogram_id program)
{
	return program->extractChangeDetail();
}

inline void MANAGER_CLEANUP_CHANGE_DETAIL(cmzn_shaderprogram)(
		cmzn_shaderprogram_change_detail **change_detail_address)
{
	delete *change_detail_address;
}

DECLARE_MANAGER_UPDATE_FUNCTION(cmzn_shaderprogram)

DECLARE_MANAGER_FIND_CLIENT_FUNCTION(cmzn_shaderprogram)

DECLARE_MANAGED_OBJECT_NOT_IN_USE_CONDITIONAL_FUNCTION(cmzn_shaderprogram)

PROTOTYPE_ACCESS_OBJECT_FUNCTION(cmzn_shaderprogram)
{
	if (object)
		return object->access();
	return 0;
}

PROTOTYPE_DEACCESS_OBJECT_FUNCTION(cmzn_shaderprogram)
{
	return cmzn_shaderprogram::deaccess(object_address);
}

PROTOTYPE_REACCESS_OBJECT_FUNCTION(cmzn_shaderprogram)
{
	if (object_address)
	{
		if (new_object)
		{
			new_object->access();
		}
		if (*object_address)
		{
			cmzn_shaderprogram::deaccess(object_address);
		}
		*object_address = new_object;
		return 1;
	}
	return 0;
}


DECLARE_DEFAULT_GET_OBJECT_NAME_FUNCTION(cmzn_shaderprogram)

DECLARE_INDEXED_LIST_STL_FUNCTIONS(cmzn_shaderprogram)
DECLARE_FIND_BY_IDENTIFIER_IN_INDEXED_LIST_STL_FUNCTION(cmzn_shaderprogram,name,const char *)
DECLARE_INDEXED_LIST_STL_IDENTIFIER_CHANGE_FUNCTIONS(cmzn_shaderprogram,name)

DECLARE_MANAGER_FUNCTIONS(cmzn_shaderprogram,manager)
DECLARE_DEFAULT_MANAGED_OBJECT_NOT_IN_USE_FUNCTION(cmzn_shaderprogram,manager)
DECLARE_MANAGER_IDENTIFIER_WITHOUT_MODIFY_FUNCTIONS(cmzn_shaderprogram,name,const char *,manager)
DECLARE_MANAGER_OWNER_FUNCTIONS(cmzn_shaderprogram, struct cmzn_shadermodule)


#if defined (OPENGL_API)
int cmzn_shaderprogram_compile(cmzn_shaderprogram_id shader_program,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 4 July 2007

DESCRIPTION :
Compiles the shader program objects.  These are separate objects so they can
be shared by multiple materials using the same program.
==============================================================================*/
{
	 int return_code;
	ENTER(cmzn_shaderprogram_compile);
	if (shader_program)
	{
#if defined (OPENGL_API)
		/* #define TESTING_PROGRAM_STRINGS */
/* #define WRITE_STRING */
/* #define DEBUG_CODE */
#if defined (DEBUG_CODE)
		const GLubyte *error_msg;
#endif /* defined (DEBUG_CODE) */
		return_code = 1;
#if defined (TESTING_PROGRAM_STRINGS)
		/* If testing always recompile */
		shader_program->compiled = 0;
#endif /* defined (TESTING_PROGRAM_STRINGS) */
		if (!shader_program->compiled)
		{
#if (defined GL_ARB_vertex_program && defined GL_ARB_fragment_program) || defined GL_VERSION_2_0
#if defined (GL_VERSION_2_0)
			 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_ARB)
			 {
				 if (Graphics_library_check_extension(GL_shading_language))
				 {
					 shader_program->shader_type=SHADER_PROGRAM_SHADER_GLSL;
				 }
			 }
#endif
#if defined GL_ARB_fragment_program && defined GL_ARB_vertex_program
			 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
			 {
				 if (Graphics_library_check_extension(GL_ARB_fragment_program) &&
				 Graphics_library_check_extension(GL_ARB_vertex_program))
				 {
					 shader_program->shader_type=SHADER_PROGRAM_SHADER_ARB;
				 }
			 }
#endif
			 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL ||
					 shader_program->shader_type==SHADER_PROGRAM_SHADER_ARB)
			 {
#if defined DEBUG_CODE
				printf ("Compiling program type:%x\n", shader_program->type);
#endif /* defined DEBUG_CODE */
#if defined GL_VERSION_2_0
				const char *vv, *ff, *gg;
#endif
#if ! defined (TESTING_PROGRAM_STRINGS)
				char *components_string, *fragment_program_string = NULL,
						*vertex_program_string = NULL, *geometry_program_string = NULL;
				enum Graphics_library_vendor_id vendor_id;
				const char *colour_texture_string[] = {"float", "vec2", "vec3", "vec4"};
				int colour_texture_dimension = 0, components_error, number_of_inputs,
					error;

				vendor_id = Graphics_library_get_vendor_id();
				error = 0;
				fragment_program_string = NULL;

				if (0 == shader_program->type)
				{
					vertex_program_string = duplicate_string(shader_program->vertex_program_string);
					fragment_program_string = duplicate_string(shader_program->fragment_program_string);
					if (shader_program->geometry_program_string)
					{
						geometry_program_string = duplicate_string(shader_program->geometry_program_string);
					}
					if (vertex_program_string && strstr(vertex_program_string, "!!ARBvp"))
					{
						if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_ARB)
						{
							if (Graphics_library_check_extension(GL_ARB_fragment_program) &&
								Graphics_library_check_extension(GL_ARB_vertex_program))
							{
								shader_program->shader_type=SHADER_PROGRAM_SHADER_ARB;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"Program_string is written with ARB shading program\n"
									"but ARB shading program is not supported.\n");
								vertex_program_string = NULL;
								fragment_program_string = NULL;
								geometry_program_string = NULL;
							}
						}
					}
					else if (vertex_program_string && strstr(vertex_program_string, "void main("))
					{
						if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
						{
							if (Graphics_library_check_extension(GL_shading_language))
							{
								shader_program->shader_type=SHADER_PROGRAM_SHADER_GLSL;
							}
							else
							{
								display_message(ERROR_MESSAGE,
									"Program_string is written with GLSL\n"
									"but GLSL is not supported.\n");
								vertex_program_string = NULL;
								fragment_program_string = NULL;
								geometry_program_string = NULL;
							}
						}
					}
				}

#if defined DEBUG_CODE
				if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
				{
					display_message(INFORMATION_MESSAGE,
							"OpenGL 2.0 or higher supported, GLSL supported\n");
				}
				else if (shader_program->shader_type==SHADER_PROGRAM_SHADER_ARB)
				{
					 display_message(INFORMATION_MESSAGE,
							"ARB shading program supported\n");
				}
#endif  /* defined DEBUG_CODE */

				if (SHADER_PROGRAM_CLASS_GOURAUD_SHADING & shader_program->type)
				{
					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							vertex_program_string = duplicate_string("//GOURAUDSHADING_VERTEX_SHADER\n"
								 "\n"
								 "varying vec4 NewCoord;\n"
								 "varying vec3 eyeNormal, lightVec, diffuse;\n"
								 );
					 }
					 else
					 {
							vertex_program_string = duplicate_string("!!ARBvp1.0\n"
								 "ATTRIB normal = vertex.normal;\n"
								 "ATTRIB position = vertex.position;\n"
								 "PARAM c0[4] = { state.matrix.mvp };\n"
								 "PARAM c1[4] = { state.matrix.modelview };\n"
								 "PARAM c2[4] = { state.matrix.modelview.invtrans };\n"
								 "PARAM eyeCameraPos = {0, 0, 0, 0};\n"
								 "PARAM eyeLightPos = state.light[0].position;\n"
								 "PARAM two = {2.0, 2.0, 2.0, 2.0};\n"
								 "PARAM m_one = {-1.0, -1.0, -1.0, -1.0};\n"
																											 );
					 }
					 if ((SHADER_PROGRAM_CLASS_COLOUR_TEXTURE | SHADER_PROGRAM_CLASS_SECOND_TEXTURE)
							& shader_program->type)
					 {
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"PARAM texture_scaling = program.env[0];\n"
										, &error);
							}
							else
							{
								 append_string(&vertex_program_string,
										"uniform vec4 texture_scaling;\n"
										, &error);
							}
					 }

					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							append_string(&vertex_program_string,
								 "\n"
								 "void main()\n"
								 "{\n"
								 "  vec4 eyeVertex, finalCol;\n"
								 "  float NdotHV;\n"
								 "  float Len, attenuation;\n"
								 "  eyeVertex = gl_ModelViewMatrix * gl_Vertex;\n"
								 "  eyeNormal = normalize(gl_NormalMatrix * gl_Normal);\n"
								 "  lightVec = gl_LightSource[0].position.xyz - eyeVertex.xyz;\n"
								 "  Len = length(lightVec);\n"
								 "  lightVec = normalize(lightVec);\n"
								 "  attenuation = 1.0 / (gl_LightSource[0].quadraticAttenuation * Len * Len + \n"
								 "    gl_LightSource[0].constantAttenuation +\n"
								 "    Len *  gl_LightSource[0].linearAttenuation);\n"
								 "  //NdotL = abs(dot(eyeNormal,lightVec));\n"
								 "  diffuse = vec3(gl_LightSource[0].diffuse * gl_Color *attenuation);\n"
								 "  NdotHV = abs(dot(eyeNormal, gl_LightSource[0].halfVector.xyz));\n"
								 "  finalCol = gl_FrontLightProduct[0].ambient\n"
								 "    + pow(NdotHV, gl_FrontMaterial.shininess) * gl_LightSource[0].specular\n"
								 "    * gl_FrontMaterial.specular;\n"
								 "  finalCol = finalCol * attenuation + gl_FrontMaterial.emission\n"
								 "     + gl_FrontMaterial.ambient * gl_LightModel.ambient;\n"
								"  finalCol.a = gl_LightSource[0].diffuse.a * gl_Color.a;\n"
								 , &error);
					 }
					 else
					 {
							append_string(&vertex_program_string,
								 "TEMP eyeVertex;\n"
								 "TEMP eyeNormal;\n"
								 "TEMP temp_col;\n"
								 "TEMP temp_col2;\n"
								 "TEMP lightVec, Len, finalCol, attenuation, lightContrib;\n"
								 "\n"
								 "#Vertex position in eyespace\n"
								 "DP4 eyeVertex.x, c1[0], position;\n"
								 "DP4 eyeVertex.y, c1[1], position;\n"
								 "DP4 eyeVertex.z, c1[2], position;\n"
								 "\n"
								 "DP4 eyeNormal.x, c2[0], normal;\n"
								 "DP4 eyeNormal.y, c2[1], normal;\n"
								 "DP4 eyeNormal.z, c2[2], normal;\n"
								 "\n"
								 "DP3 eyeNormal.w, eyeNormal, eyeNormal;\n"
								 "RSQ eyeNormal.w, eyeNormal.w;\n"
								 "MUL eyeNormal.xyz, eyeNormal.w, eyeNormal;\n"
								 "\n"
								 "SUB lightVec, eyeLightPos, eyeVertex;\n"
								 "#MOV lightVec, state.light[0].position;\n"
								 "\n"
								 "#Normalize lightvec and viewvec.\n"
								 "DP3		Len.w, lightVec, lightVec;\n"
								 "RSQ		lightVec.w, Len.w;\n"
								 "MUL		lightVec.xyz, lightVec, lightVec.w;\n"
								 "\n"
								 "#Calculate attenuation.\n"
								 "MAD		attenuation, state.light[0].attenuation.z, Len.w, state.light[0].attenuation.x;\n"
								 "RCP		Len, lightVec.w;\n"
								 "MAD		attenuation, Len.w, state.light[0].attenuation.y, attenuation.x;\n"
								 "RCP		attenuation.x, attenuation.x;\n"
								 "\n"
								 "#Diffuse\n"
								 "DP3      lightContrib.x, eyeNormal, lightVec;\n"
								 "ABS      lightContrib.x, lightContrib.x;\n"
								 "\n"
								 "#Specular\n"
								 "#Phong:\n"
								 "#TEMP viewVec, reflVec;\n"
								 "#SUB viewVec, eyeCameraPos, eyeVertex;\n"
								 "#DP3 viewVec.w, viewVec, viewVec;\n"
								 "#RSQ viewVec.w, viewVec.w;\n"
								 "#MUL viewVec.xyz, viewVec.w, viewVec;\n"
								 "#DP3		reflVec, lightVec, eyeNormal;\n"
								 "#MUL		reflVec, reflVec, two;\n"
								 "#MAD		reflVec, reflVec.x, eyeNormal, -lightVec;\n"
								 "#DP3	lightContrib.y, reflVec, viewVec;\n"
								 "\n"
								 "DP3      lightContrib.y, eyeNormal, state.light[0].half;\n"
								 "#new_code: should use an absolute value for the specular dot product too.\n"
								 "ABS      lightContrib.y, lightContrib.y;\n"
								 "\n"
								 "MOV		lightContrib.w, state.material.shininess.x;\n"
								 "\n"
								 "#Accelerates lighting computations\n"
								 "LIT	lightContrib, lightContrib;\n"
								 "\n"
								 "#old_code: the old code used the vertex.colour only and not takinga account of\n"
								 "#old_code: the lighting\n"
								 "#MAD		finalCol, lightContrib.y, vertex.color, state.lightprod[0].ambient;\n"
								 "#MAD		finalCol, lightContrib.z, state.lightprod[0].specular, finalCol;\n"
								 "#new_code\n"
								 "MUL    temp_col,  state.light[0].diffuse, vertex.color;\n"
								 "MAD		finalCol, lightContrib.y, temp_col, state.lightprod[0].ambient;\n"
								 "MOV    temp_col2, state.lightprod[0].specular;\n"
								 "MAD		finalCol.xyz, lightContrib.z, temp_col2, finalCol;\n"
								 "\n"
								 "MAD		finalCol, finalCol, attenuation.x, state.material.emission;\n"
								 "#ADD		finalCol, finalCol, state.lightmodel.scenecolor;\n"
								 "\n"
								 "#old_code:\n"
								 "MAD		finalCol, state.material.ambient, state.lightmodel.ambient, finalCol;\n"
								 "#MAD		finalCol, vertex.color, state.lightmodel.ambient, finalCol;\n"
								 "\n"
								 "#old_code\n"
								 "#MOV finalCol.w, state.material.diffuse.w;\n"
								 "#new_code\n"
								 "MOV finalCol.w, temp_col.w;\n"
								 "\n"
								 , &error);
					 }

					 if ((SHADER_PROGRAM_CLASS_COLOUR_TEXTURE | SHADER_PROGRAM_CLASS_SECOND_TEXTURE)
							& shader_program->type)
					 {
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"MUL result.texcoord[0], texture_scaling, vertex.texcoord[0];\n"
										, &error);
							}
							else
							{
								 append_string(&vertex_program_string,
										"gl_TexCoord[0] = texture_scaling * gl_MultiTexCoord0;\n"
										, &error);
							}
					 }

					 if(shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							append_string(&vertex_program_string,
								 "  gl_FrontColor = finalCol;\n"
								 "  gl_BackColor = finalCol;\n"
								 "  NewCoord = ftransform();\n"
								 "  gl_Position = NewCoord;\n"
								 "}\n"
								 , &error);
					 }
					 else
					 {
							append_string(&vertex_program_string,
								 "DP4 result.texcoord[1].x, c0[0], position;\n"
								 "DP4 result.texcoord[1].y, c0[1], position;\n"
								 "DP4 result.texcoord[1].z, c0[2], position;\n"
								 "DP4 result.texcoord[1].w, c0[3], position;\n"
								 "\n"
								 "MOV result.color, finalCol;\n"
								 "MOV result.color.back, finalCol;\n"
								 "#MOV result.color.secondary,  {1, 1, 1, 1};\n"
								 "#MOV result.color.back.secondary,  {0, 0, 0, 0};\n"
								 "DP4 result.position.x, c0[0], position;\n"
								 "DP4 result.position.y, c0[1], position;\n"
								 "DP4 result.position.z, c0[2], position;\n"
								 "DP4 result.position.w, c0[3], position;\n"
								 "\n"
								"END\n"
								 , &error);
					 }

					 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type)
					 {
							/* Set the colour texture dimension here so that I can use it when
							 defining uniform variable in GLSL*/
							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 & shader_program->type)
							{
								 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2 & shader_program->type)
								 {
										colour_texture_dimension = 3;
								 }
								 else
								 {
										colour_texture_dimension = 1;
								 }
							}
							else
							{
								 colour_texture_dimension = 2;
							}
					 }
					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							fragment_program_string = duplicate_string("//fragment shader\n");
					 }
					 else
					 {
							fragment_program_string = duplicate_string("!!ARBfp1.0\n");
					 }

					 if (SHADER_PROGRAM_CLASS_ORDER_INDEPENDENT_PEEL_LAYER & shader_program->type)
					 {
							if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&fragment_program_string,
										"#extension GL_ARB_texture_rectangle : enable\n"
										"uniform vec4 texturesize;\n"
										, &error);
								 if (Graphics_library_vendor_mesa != vendor_id)
								 {
										append_string(&fragment_program_string,
											 "uniform sampler2DRectShadow samplertex;\n"
											 , &error);
								 }
								 else
								 {
										append_string(&fragment_program_string,
											 "uniform sampler2DRect samplertex;\n"
											 , &error);
								 }
							}
							else
							{
								 if (Graphics_library_vendor_mesa != vendor_id)
								 {
										append_string(&fragment_program_string,
											 "OPTION ARB_fragment_program_shadow;\n"
											 , &error);
								 }
								 append_string(&fragment_program_string,
										"PARAM texturesize = program.env[1];\n"
										"TEMP tex4, kill, tex4coord;\n"
										, &error);
							}
					 }

					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type)
							{
								 char temp_string[100];
								 sprintf(temp_string,
										"uniform sampler%dD texture0;\n", colour_texture_dimension);
								 append_string(&fragment_program_string,
										temp_string, &error);
							}
							append_string(&fragment_program_string,
								 "varying vec4 NewCoord;\n"
								 "varying vec3 eyeNormal, lightVec, diffuse;\n"
								 "\n"
								 "void main()\n"
								 "{\n"
								 "  float perspective, texel, NdotL;\n"
								 "  vec4 tex4coord, eyespaceCoord, finalCol;\n"
								 "  perspective = float(1.0) / NewCoord.w;\n"
								 "  eyespaceCoord = NewCoord;\n"
								 "  eyespaceCoord = eyespaceCoord * perspective * 0.5 + 0.5;\n"
								 "  NdotL = dot(normalize(eyeNormal),normalize(lightVec));\n"
								 "  if (!gl_FrontFacing)\n"
								 "    NdotL = abs(NdotL);\n"
								 "  finalCol.xyz = vec3(NdotL * diffuse + gl_Color.xyz);\n"
								 "  finalCol.a = gl_Color.a;\n"
								 , &error);
					 }
					 else
					 {
							append_string(&fragment_program_string,
								 "TEMP eyespaceCoord, perspective;\n"
								 "PARAM point_five = {0.5, 0.5, 0.5, 0.5};\n"

								 "MOV      eyespaceCoord, fragment.texcoord[1];\n"
								 "RCP      perspective.w, eyespaceCoord.w;\n"
								 "MUL      eyespaceCoord, eyespaceCoord, perspective.w;\n"
								 "MAD      eyespaceCoord, eyespaceCoord, point_five, point_five;\n"
								 "MOV      eyespaceCoord.w, 1.0;\n"
								 , &error);
					 }

					 if (SHADER_PROGRAM_CLASS_ORDER_INDEPENDENT_PEEL_LAYER & shader_program->type)
					 {
							if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
							{
								 if (Graphics_library_vendor_mesa == vendor_id)
								 {
										append_string(&fragment_program_string,
											 "  tex4coord = texturesize * eyespaceCoord.xyzx;\n"
											 "  tex4coord.z = gl_FragCoord.z;\n"
											 "  texel = eyespaceCoord.z;\n"
											 "  //texel = texture2DRect(samplertex, vec3(tex4coord)).x\n"
											 "  if (texel >= 0)\n"
											 "  {\n"
											 "    discard;\n"
											 "  }\n"
											 , &error);
								 }
								 else
								 {
										if (Graphics_library_vendor_ati == vendor_id)
										{
											 append_string(&fragment_program_string,
													"  tex4coord = texturesize * eyespaceCoord.xyzx;\n"
													"  tex4coord.z = gl_FragCoord.z -0.0001;\n"
													, &error);
										}
										else
										{
											 append_string(&fragment_program_string,
													"  tex4coord = texturesize * eyespaceCoord.xyzx;\n"
													, &error);
										}
										append_string(&fragment_program_string,
											 "  texel =  shadow2DRect(samplertex, vec3(tex4coord)).r;\n"
													"  if (texel<0.5)\n"
											 "  {\n"
											 "    discard;\n"
											 "  }\n"
											 , &error);
								 }
							}
							else
							{
								 if (Graphics_library_vendor_mesa == vendor_id)
								 {
										append_string(&fragment_program_string,
											 "MOV      tex4coord, eyespaceCoord.xyzx;\n"
											 "MUL      tex4coord, tex4coord, texturesize;\n"
											 "MOV     tex4coord.z, fragment.position.z;\n"
											 "TEX		tex4.x, tex4coord, texture[3], RECT;\n"
											 "MOV     kill, tex4.xxxx;\n"
											 "KIL     -kill;\n"
											 , &error);
								 }
								 else
								 {
										if (Graphics_library_vendor_ati == vendor_id)
										{
											 append_string(&fragment_program_string,
													"MOV      tex4coord, eyespaceCoord.xyzx;\n"
													"MUL      tex4coord, tex4coord, texturesize;\n"
													"\n"
													"MOV     tex4coord.z, fragment.position.z;\n"
													"ADD     tex4coord.z, tex4coord.z, -0.0001;\n"
													, &error);
										}
										else
										{
											 /* Default is what we used to have which is for Nvidia */
											 append_string(&fragment_program_string,
													"MOV      tex4coord, eyespaceCoord.xyzx;\n"
													"MUL      tex4coord, tex4coord, texturesize;\n"
													"\n"
													"MOV      tex4.x, eyespaceCoord.zzzz;\n"
													, &error);
										}
										append_string(&fragment_program_string,
											 "TEX		tex4.x, tex4coord, texture[3], SHADOWRECT;\n"
											 "ADD      kill.x, tex4.x, -0.5;\n"
											 "KIL      kill.x;\n"
											 , &error);
								 }
							}
					 }
					 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type)
					 {
							char tex_string[100];
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 /* Load the colour texture */
								 sprintf(tex_string,
										"TEMP		tex;\n"
										"TEX		tex, fragment.texcoord[0], texture[0], %dD;\n",
										colour_texture_dimension);
								 append_string(&fragment_program_string,
										tex_string, &error);
							}
							else
							{
								 /* Load the colour texture */
								 if (colour_texture_dimension > 1)
								 {
										sprintf(tex_string,
											 "  vec4 tex = texture%dD(texture0, %s(gl_TexCoord[0]));\n",
											 colour_texture_dimension, colour_texture_string[colour_texture_dimension - 1]);
								 }
								 append_string(&fragment_program_string,
										tex_string, &error);
							}
					 }
					 else
					 {
							colour_texture_dimension = 0;
					 }

					 if (!(SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type))
					 {
							if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
							{
								append_string(&fragment_program_string,
									 "  gl_FragColor = finalCol;\n"
									 , &error);
							}
							else
							{
								 /* Normal lighting, just use the fragment colour */
								 append_string(&fragment_program_string,
										"MOV      result.color.xyzw, fragment.color.rgba;\n"
										, &error);
							}
					 }
					 else
					 {
							if (!(SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_DECAL & shader_program->type))
							{
								 /* Modulate */
								 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 & shader_program->type)
								 {
										if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
										{
											 /* RGB texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MUL		result.color.xyz, fragment.color, tex;\n"
														 "MOV	  result.color.w, fragment.color.w;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "  gl_FragColor.rgb = vec3(finalCol * tex);\n"
														 "  gl_FragColor.a = finalCol.a;\n"
														 , &error);
											 }
										}
										else
										{
											 /* grayscale texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MUL		result.color.xyz, fragment.color.xyz, tex.x;\n"
														 "MOV	  result.color.w, fragment.color.w;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "  gl_FragColor.rgb = finalCol.rgb * tex.x;\n"
														 "  gl_FragColor.a = finalCol.a;\n"
														 , &error);
											 }
										}
								 }
								 else
								 {
										if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
										{
											 /* grayscale alpha texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MUL		result.color.xyz, fragment.color.xyz, tex.x;\n"
														 "MUL		result.color.w, fragment.color.w, tex.y;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "  gl_FragColor.rgb = finalCol.xyz * tex.x;\n"
														 "  gl_FragColor.a = finalCol.a * tex.y;\n"
														 , &error);
											 }
										}
										else
										{
											 /* RGBA texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MUL		result.color, fragment.color, tex;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "  gl_FragColor = finalCol * tex;\n"
														 , &error);
											 }
										}
								 }
							}
							else
							{
								 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 & shader_program->type)
								 {
										if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
										{
											 /* RGB texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MOV		result.color.xyz, tex;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "	gl_FragColor.rgb = tex.xyz;\n"
														 , &error);
											 }
										}
										else
										{
											 /* grayscale texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MOV		result.color.xyz, tex.x;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "	gl_FragColor.rgb = tex.xxx;\n"
														 , &error);
											 }
										}
										if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
										{
											 append_string(&fragment_program_string,
													"MOV		result.color.w, state.material.diffuse.w;\n"
													, &error);
										}
										else
										{
											 append_string(&fragment_program_string,
													"  gl_FragColor.a = gl_FrontMaterial.diffuse.w;\n"
													, &error);
										}
								 }
								 else
								 {
										if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
										{
											 /* grayscale alpha texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MOV		result.color.xyz, tex.x;\n"
														 "MOV		result.color.w, tex.y;\n"
														 , &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "  gl_FragColor.xyz = tex.x;\n"
														 "  gl_FragColor.w = tex.y;\n"
														 , &error);
											 }
										}
										else
										{
											 /* RGBA texture */
											 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
											 {
													append_string(&fragment_program_string,
														 "MOV		result.color, tex;\n"
													, &error);
											 }
											 else
											 {
													append_string(&fragment_program_string,
														 "  gl_FragColor = tex;\n"
													, &error);
											 }
										}
								 }
							}
					 }
					 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
					 {
							append_string(&fragment_program_string,
								 "MOV		 result.depth.z, eyespaceCoord.z;\n"
								 "\n"
								 "END\n"
								 , &error);
					 }
					 else
					 {
							append_string(&fragment_program_string,
								 "  gl_FragDepth = eyespaceCoord.z;\n"
								 "}\n"
								 , &error);
					 }
				}
				else if (SHADER_PROGRAM_CLASS_PER_PIXEL_LIGHTING & shader_program->type)
				{
					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							vertex_program_string = duplicate_string("//Vertex Shader\n"
																											 "#version 110\n");
							append_string(&vertex_program_string,
								 "varying vec4 diffuse, ambientGlobal, ambient;\n"
								 , &error);
					 }
					 else
					 {
							vertex_program_string = duplicate_string("!!ARBvp1.0\n");
							append_string(&vertex_program_string,
								 "ATTRIB normal = vertex.normal;\n"
								 "ATTRIB position = vertex.position;\n"
								 , &error);
					 }
					 if ((SHADER_PROGRAM_CLASS_COLOUR_TEXTURE | SHADER_PROGRAM_CLASS_SECOND_TEXTURE)
							& shader_program->type)
					 {
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"PARAM texture_scaling = program.env[0];\n"
										, &error);
							}
							else
							{
								 append_string(&vertex_program_string,
								 "uniform vec4 texture_scaling;\n"
								 , &error);
							}
					 }
					 if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & shader_program->type)
					 {
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"ATTRIB tangent = vertex.texcoord[1];\n"
										, &error);
							}
					 }
					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							append_string(&vertex_program_string,
								 "\nvoid main()\n"
								 "{\n"
								 "  vec3 pos;\n"
								 , &error);
					 }
					 else
					 {
							append_string(&vertex_program_string,
								 "PARAM c0[4] = { state.matrix.mvp };\n"
								 "PARAM c1[4] = { state.matrix.modelview };\n"
								 "PARAM eyeCameraPos = {0, 0, 0, 0};\n"
								 "PARAM eyeLightPos = state.light[0].position;\n"

								 "TEMP eyeVertex;\n"
								 "TEMP viewVec;\n"
								 , &error);
					 }
					 if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & shader_program->type)
					 {
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"PARAM c3[4] = { state.matrix.modelview.inverse };\n"
										"TEMP lightVec;\n"
										"TEMP objectLight;\n"
										"TEMP cameraVec;\n"
										"TEMP objectCamera;\n"
										"TEMP binormal;\n"
										, &error);
							}
					 }
					 else
					 {
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)

							{
								 append_string(&vertex_program_string,
										"PARAM c2[4] = { state.matrix.modelview.invtrans };\n"
										"TEMP eyeNormal;\n"
										, &error);
							}
					 }

					 if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					 {
							append_string(&vertex_program_string,
								 "  diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;\n"
								 "  ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;\n"
								 "  ambientGlobal = gl_LightModel.ambient * gl_FrontMaterial.ambient;\n"
								 "  vec4 ecPos = gl_ModelViewMatrix * gl_Vertex;\n"
								 "  vec3 aux = gl_LightSource[0].position.xyz - ecPos.xyz;\n"
								 , &error);
					 }
					 else
					 {
							append_string(&vertex_program_string,
								 "#Vertex position in eyespace\n"
								 "DP4 eyeVertex.x, c1[0], position;\n"
								 "DP4 eyeVertex.y, c1[1], position;\n"
								 "DP4 eyeVertex.z, c1[2], position;\n"
								 , &error);
					 }

					if ((SHADER_PROGRAM_CLASS_COLOUR_TEXTURE | SHADER_PROGRAM_CLASS_SECOND_TEXTURE)
						& shader_program->type)
					{
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"MUL result.texcoord[0], texture_scaling, vertex.texcoord[0];\n"
										, &error);
							}
							else
							{
								 append_string(&vertex_program_string,
										"  gl_TexCoord[0] = texture_scaling * gl_MultiTexCoord0;\n"
										, &error);
							}
					}

					if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & shader_program->type)
					{
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"MUL binormal.xyz, tangent.zxyz, normal.yzxy;\n"
										"MAD binormal.xyz, tangent.yzxy, normal.zxyz, -binormal.xyzx;\n"

										"SUB lightVec, eyeLightPos, eyeVertex;\n"

										"DP3 objectLight.x, c3[0], lightVec;\n"
										"DP3 objectLight.y, c3[1], lightVec;\n"
										"DP3 objectLight.z, c3[2], lightVec;\n"

										"DP3 result.texcoord[1].x, tangent, objectLight;\n"
										"DP3 result.texcoord[1].y, binormal, objectLight;\n"
										"DP3 result.texcoord[1].z, normal, objectLight;\n"

										"SUB cameraVec, eyeCameraPos, eyeVertex;\n"
										"DP3 objectCamera.x, c3[0], cameraVec;\n"
										"DP3 objectCamera.y, c3[1], cameraVec;\n"
										"DP3 objectCamera.z, c3[2], cameraVec;\n"

										"DP3 result.texcoord[2].x, tangent, objectCamera;\n"
										"DP3 result.texcoord[2].y, binormal, objectCamera;\n"
										"DP3 result.texcoord[2].z, normal, objectCamera;\n"
										, &error);
							}
							else
							{
								 append_string(&vertex_program_string,
										"\n"
										"  vec3 binormal = vec3(gl_MultiTexCoord1.yzxy * gl_Normal.zxyz) - \n"
									  "         vec3(gl_MultiTexCoord1.zxyz * gl_Normal.yzxy);\n"
										"  vec3 temp = vec3(gl_MultiTexCoord1);\n"
										"  mat3 TBN_Matrix = gl_NormalMatrix * mat3(temp, binormal, gl_Normal);\n"
										"  gl_TexCoord[1].xyz = aux * TBN_Matrix; \n"
										"  gl_TexCoord[2].xyz = vec3(-ecPos) *TBN_Matrix; \n"
										, &error);
							}
					}
					else
					{
							if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&vertex_program_string,
										"  gl_TexCoord[3].xyz = normalize((gl_ModelViewMatrixInverseTranspose * vec4(gl_Normal,0.0)).xyz);\n"
										"  gl_TexCoord[2].xyz = vec3(normalize(-ecPos.xyz));\n"
										"  gl_TexCoord[1].xyz = aux;\n"
										, &error);
							}
							else
							{
								 append_string(&vertex_program_string,
										"DP4 eyeNormal.x, c2[0], normal;\n"
										"DP4 eyeNormal.y, c2[1], normal;\n"
										"DP4 eyeNormal.z, c2[2], normal;\n"
										"DP3 eyeNormal.w, eyeNormal, eyeNormal;\n"
										"RSQ eyeNormal.w, eyeNormal.w;\n"
										"MUL eyeNormal.xyz, eyeNormal.w, eyeNormal;\n"

										"SUB viewVec, eyeCameraPos, eyeVertex;\n"
										"DP3 viewVec.w, viewVec, viewVec;\n"
										"RSQ viewVec.w, viewVec.w;\n"
										"MUL viewVec.xyz, viewVec.w, viewVec;\n"

										"SUB result.texcoord[1], eyeLightPos, eyeVertex;\n"
										"MOV result.texcoord[2], viewVec;\n"
										"MOV result.texcoord[3], eyeNormal;\n"
										, &error);
							}
					}

					if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					{
						 append_string(&vertex_program_string,
								"  gl_FrontColor = gl_Color;\n"
								"  gl_BackColor = gl_Color;\n"
							  "  gl_FrontSecondaryColor = vec4(1.0);\n"
							  "  gl_BackSecondaryColor = vec4(0.0);\n"
								"  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
								"}\n"
								, &error);



					}
					else
					{
						 append_string(&vertex_program_string,
								"MOV result.color, vertex.color;\n"
								"MOV result.color.back, vertex.color;\n"
								"MOV result.color.secondary,  {1, 1, 1, 1};\n"
								"MOV result.color.back.secondary,  {0, 0, 0, 0};\n"
								"DP4 result.position.x, c0[0], position;\n"
								"DP4 result.position.y, c0[1], position;\n"
								"DP4 result.position.z, c0[2], position;\n"
								"DP4 result.position.w, c0[3], position;\n"
								"END\n"
								, &error);
					}
					if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					{
						 fragment_program_string = duplicate_string("//fragment shader\n"
																												"#version 110\n");
						 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type)
						 {
								if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 & shader_program->type)
								{
									 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2 & shader_program->type)
									 {
											append_string(&fragment_program_string,
												 "uniform sampler3D texture0;\n"
												 , &error);
									 }
									 else
									 {
											append_string(&fragment_program_string,
												 "uniform sampler1D texture0;\n"
												 , &error);
									 }
								}
								else
								{
											append_string(&fragment_program_string,
												 "uniform sampler2D texture0;\n"
												 , &error);
								}
						 }

						 if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE & shader_program->type)
						 {
								/* Load the second texture using the same texture coordinates as the colour texture */
								if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_1 & shader_program->type)
								{
									 if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_2 & shader_program->type)
									 {
											append_string(&fragment_program_string,
												 "uniform sampler3D texture1;\n"
												 , &error);
									 }
									 else
									 {
											append_string(&fragment_program_string,
												 "uniform sampler1D texture1;\n"
												 , &error);
									 }
								}
								else
								{
											append_string(&fragment_program_string,
												 "uniform sampler2D texture1;\n"
												 , &error);
								}
						 }

						 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1D_COMPONENT_LOOKUP & shader_program->type)
						 {
							 append_string(&fragment_program_string,
								 "uniform sampler1D texture1;\n"
								 , &error);
						 }

						 if ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1 |
									 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2 |
									 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3 |
									 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4) & shader_program->type)
						 {
								if ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR |
											SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA) & shader_program->type)
								{									 number_of_inputs = 0;
									 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1
											& shader_program->type)
									 {
											number_of_inputs++;
									 }
									 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2
									 & shader_program->type)
									 {
											number_of_inputs++;
									 }
									 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3
											& shader_program->type)
									 {
											number_of_inputs++;
									 }
									 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4
									 & shader_program->type)
									 {
											number_of_inputs++;
									 }
									 if (number_of_inputs > 0 && number_of_inputs < 4)
									 {
											char new_string[1000];
											sprintf(new_string,
												 "uniform sampler%1dD texture1;\n", number_of_inputs);
											append_string(&fragment_program_string,
												 new_string, &error);
									 }
								}
								append_string(&fragment_program_string,
									 "uniform vec4 lookup_offsets, lookup_scales;\n"
									 , &error);
						 }

						 if ((SHADER_PROGRAM_CLASS_LIT_VOLUME_INTENSITY_NORMAL_TEXTURE |
									 SHADER_PROGRAM_CLASS_LIT_VOLUME_FINITE_DIFFERENCE_NORMAL) &
								shader_program->type)
						 {
								append_string(&fragment_program_string,
									 "uniform vec4 texture_scaling, normal_scaling;\n"
									 , &error);
						 }
						 append_string(&fragment_program_string,
								"varying vec4 diffuse, ambientGlobal, ambient;\n"
								"\n"
								"void main()\n"
								"{\n"
								"  vec4 color;\n"
								"  vec3 n, reflV, viewV, ldir;\n"
								"  float NdotL, NdotHV, len;\n"
								"  float att;\n"
								"\n"
								, &error);
					}
					else
					{
						 fragment_program_string = duplicate_string("!!ARBfp1.0\n");
						 append_string(&fragment_program_string,
								"TEMP lightVec, viewVec, reflVec, normal, attenuation, Len, finalCol, lightContrib, reverse, tex, tex2;\n"
								"PARAM two = {2.0, 2.0, 2.0, 2.0};\n"
								"PARAM m_one = {-1.0, -1.0, -1.0, -1.0};\n"
								, &error);
					}

					if ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1 |
							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2 |
							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3 |
							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4) & shader_program->type)
					{
						 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
						 {
								append_string(&fragment_program_string,
									 "PARAM lookup_offsets = program.env[1];\n"
									 "PARAM lookup_scales = program.env[2];\n"
									 , &error);
						 }
					}

					if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					{
						 append_string(&fragment_program_string,
								"  n = normalize(gl_TexCoord[3].xyz);\n"
								, &error);
					}
					else
					{
						 append_string(&fragment_program_string,
								"#Set up reverse vector based on secondary colour\n"
								"MAD      reverse, two, fragment.color.secondary.x, m_one;\n"
								, &error);
					}

					if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type)
					{
						int colour_texture_string_index = 0;
						char tex_string[100];
						if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
						{
							 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 & shader_program->type)
							 {
									if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
									{
										 colour_texture_string_index = 2;
									}
									else
									{
										 colour_texture_string_index = 0;
									}
							 }
							 else
							 {
									if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
									{
										 colour_texture_string_index = 1;
									}
									else
									{
										 colour_texture_string_index = 3;
									}
							 }
						}
						/* Load the colour texture */
						if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 & shader_program->type)
						{
							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2 & shader_program->type)
							{
								 /* RGB texture */
								 colour_texture_dimension = 3;

							}
							else
							{
								colour_texture_dimension = 1;
							}

						}
						else
						{
							 colour_texture_dimension = 2;

						}
						if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
						{
							 sprintf(tex_string,
									"  %s tex = %s(texture%dD(texture0, %s(gl_TexCoord[0])));\n",
									colour_texture_string[colour_texture_string_index],
									colour_texture_string[colour_texture_string_index],
								  colour_texture_dimension,
									colour_texture_string[colour_texture_dimension - 1]);
						}
						else
						{
							 sprintf(tex_string,
									"TEX		tex, fragment.texcoord[0], texture[0], %dD;\n",
									colour_texture_dimension);
						}
						append_string(&fragment_program_string,
							 tex_string, &error);
					}
					else
					{
						colour_texture_dimension = 0;
					}

					if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE & shader_program->type)
					{
						/* Load the second texture using the same texture coordinates as the colour texture */
						if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_1 & shader_program->type)
						{
							if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_2 & shader_program->type)
							{
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
										append_string(&fragment_program_string,
											 "TEX		tex2, fragment.texcoord[0], texture[1], 3D;\n"
											 , &error);
								 }
								 else
								 {
										append_string(&fragment_program_string,
											 "  vec3 tex2 = texture3D(texture1, gl_TexCoord[0].xyz).rgb;\n"
											 , &error);
								 }
							}
							else
							{
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
										append_string(&fragment_program_string,
											 "TEX		tex2, fragment.texcoord[0], texture[1], 1D;\n"
											 , &error);
								 }
								 else
								 {
										append_string(&fragment_program_string,
											 "  vec3 tex2 = texture1D(texture1, gl_TexCoord[0].x).rgb;\n"
											 , &error);
								 }
							}
						}
						else
						{
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "TEX		tex2, fragment.texcoord[0], texture[1], 2D;\n"
										 , &error);
							 }
							 else
							 {
									append_string(&fragment_program_string,
										 "  vec3 tex2 = texture2D(texture1, gl_TexCoord[0].xy).rgb;\n"
										 , &error);
							 }
						}
					}
					if (!(SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_DECAL & shader_program->type))
					{
						if (SHADER_PROGRAM_CLASS_SECOND_TEXTURE_BUMPMAP & shader_program->type)
						{
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "#Expand the range of the normal texture\n"
										 "MAD      normal, two, tex2, m_one;\n"

										 "#Reverse the texture normal direction component if required\n"
										 "MUL      normal.z, reverse.z, normal.z;\n"
										 , &error);
							 }
							 else
							 {
									append_string(&fragment_program_string,
										 "//Expand the range of the normal texture\n"
										 "  n = 2.0 * tex2 - 1.0;\n"
										 "  n = normalize(n);\n"
										 "//Reverse the texture normal direction component if required\n"
										 "  if (!gl_FrontFacing)\n"
										 "    n.z = -1.0 * n.z;\n"
										 , &error);
							 }
						}
						else
						{
							/* Normal is stored in texcoord[3] */
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "#Normalize the normal.\n"
										 "DP3		normal.w, fragment.texcoord[3], fragment.texcoord[3];\n"
										 "RSQ		normal.w, normal.w;\n"
										 "MUL		normal.xyz, fragment.texcoord[3], normal.w;\n"

										 "#Reverse the normal if required\n"
										 "MUL      normal, reverse, normal;\n"
										 , &error);
							 }
						}
						/* Usual lighting calculations */
						if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
						{
							 append_string(&fragment_program_string,
									"  color = gl_Color * gl_LightModel.ambient + gl_FrontMaterial.emission;\n"
									"  len = length(vec3(gl_TexCoord[1]));\n"
									"  NdotL = (dot(n, normalize(gl_TexCoord[1].xyz)));\n"
									"  if (!gl_FrontFacing)\n"
									"    NdotL = abs(NdotL);\n"
									"\n"
									"  att = 1.0 / (gl_LightSource[0].constantAttenuation +\n"
									"    gl_LightSource[0].linearAttenuation * len +\n"
									"    gl_LightSource[0].quadraticAttenuation * len * len);\n"
									"  color += att * (diffuse *NdotL + ambient);\n"
									"\n"
									"  reflV = reflect(-normalize(gl_TexCoord[1].xyz), n);\n"
									"  NdotHV = max(dot(reflV, normalize(gl_TexCoord[2].xyz)),0.0);\n"
									"  color += att * gl_FrontMaterial.specular * gl_LightSource[0].specular *\n"
									"    pow(NdotHV, gl_FrontMaterial.shininess);\n"
									"  color.w = gl_FrontMaterial.diffuse.w;\n"
									, &error);
						}
						else
						{
							 append_string(&fragment_program_string,
									"#Normalize lightvec and viewvec.\n"
									"DP3		Len.w, fragment.texcoord[1], fragment.texcoord[1];\n"
									"RSQ		lightVec.w, Len.w;\n"
									"MUL		lightVec.xyz, fragment.texcoord[1], lightVec.w;\n"

									"DP3		viewVec.w, fragment.texcoord[2], fragment.texcoord[2];\n"
									"RSQ		viewVec.w, viewVec.w;\n"
									"MUL		viewVec.xyz, fragment.texcoord[2], viewVec.w;\n"

									"#Calculate attenuation.\n"
									"MAD		attenuation, state.light[0].attenuation.z, Len.w, state.light[0].attenuation.x;\n"
									"RCP		Len, lightVec.w;\n"
									"MAD		attenuation, Len.w, state.light[0].attenuation.y, attenuation.x;\n"
									"RCP		attenuation.x, attenuation.x;\n"

									"#Diffuse\n"
									"DP3_SAT	   lightContrib.x, normal, lightVec;\n"
									"\n"
									"#Specular\n"
									"# Phong:\n"
									"DP3		reflVec, lightVec, normal;\n"
									"MUL		reflVec, reflVec, two;\n"
									"MAD		reflVec, reflVec, normal, -lightVec;\n"
									"\n"
									"DP3_SAT	lightContrib.y, reflVec, viewVec;\n"

									"MOV		lightContrib.w, state.material.shininess.x;\n"

									"#Accelerates lighting computations\n"
									"LIT	lightContrib, lightContrib;\n"

									"MAD		finalCol, lightContrib.y, fragment.color, state.lightprod[0].ambient;\n"
									"MAD		finalCol, lightContrib.z, state.lightprod[0].specular, finalCol;\n"
									"MAD		finalCol, finalCol, attenuation.x, state.material.emission;\n"

									"#Ambient lighting contribution;\n"
									"MAD		finalCol, fragment.color, state.lightmodel.ambient, finalCol;\n"

									"#Alpha value;\n"
									"MOV		finalCol.w, state.material.diffuse.w;\n"
									, &error);
						}

						/* The Ambient lighting contribution is using the fragment.color which
							is derived from the diffuse component rather than the ambient one.
							Should probably pass the ambient material colour through as a different
							colour */
						if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE & shader_program->type)
						{
							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 & shader_program->type)
							{
								if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
								{
									/* RGB texture */
									 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
									 {
											append_string(&fragment_program_string,
												 "MUL		finalCol.xyz, finalCol, tex;\n"
												 , &error);
									 }
									 else
									 {
											append_string(&fragment_program_string,
												 "  color.xyz = color.xyz * tex.xyz;\n"
												 , &error);
									 }
								}
								else
								{
									 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
									 {
											/* grayscale texture */
											append_string(&fragment_program_string,
												 "MUL		finalCol.xyz, finalCol.xyz, tex.x;\n"
												 , &error);
									 }
									 else
									 {
											/* float type tex */
											append_string(&fragment_program_string,
												 "  color.xyz = color.xyz * tex;\n"
												 , &error);
									 }
								}
							}
							else
							{
								 if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
								 {
										if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
										{
											 /* grayscale alpha texture */
											 append_string(&fragment_program_string,
													"MUL		finalCol.xyz, finalCol.xyz, tex.x;\n"
													"MUL		finalCol.w, finalCol.w, tex.y;\n"
													, &error);
										}
										else
										{
											append_string(&fragment_program_string,
												 "  color.xyz = color.xyz * tex.x;\n"
												 "  color.w = color.w * tex.y;\n"
												 , &error);
										}
								 }
								 else
								 {
										if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
										{
											 /* RGBA texture */
											 append_string(&fragment_program_string,
													"MUL		finalCol, finalCol, tex;\n"
													, &error);
										}
										else
										{
											append_string(&fragment_program_string,
												 "  color = color * tex;\n"
												 , &error);
										}
								 }
							}
						}
					}
					else
					{
						/* No lighting calculations are required for a decal texture */
						if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1 & shader_program->type)
						{
							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
							{
								/* RGB texture */
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
										append_string(&fragment_program_string,
											 "MOV		finalCol.xyz, tex;\n"
											 , &error);
								 }
								 else
								 {
										append_string(&fragment_program_string,
											 "  color.xyz = tex.xyz;\n"
											 , &error);
								 }
							}
							else
							{
								/* grayscale texture */
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
										append_string(&fragment_program_string,
											 "MOV		finalCol.xyz, tex.x;\n"
											 , &error);
								 }
								 else
								 {
										/* float type tex */
										append_string(&fragment_program_string,
											 "  color.xyz = vec3(tex);\n"
											 , &error);
								 }
							}
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&fragment_program_string,
										"MOV		finalCol.w, state.material.diffuse.w;\n"
										, &error);
							}
							else
							{
								 append_string(&fragment_program_string,
										"  color.w = gl_FrontMaterial.diffuse.w;\n"
										, &error);
							}
						}
						else
						{
							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2 & shader_program->type)
							{
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
										/* grayscale alpha texture */
										append_string(&fragment_program_string,
											 "MOV		finalCol.xyz, tex.x;\n"
											 "MOV		finalCol.w, tex.y;\n"
											 , &error);
								 }
								 else
								 {
										append_string(&fragment_program_string,
											 "  color.xyz = tex.xxx;\n"
											 "  color.w = tex.y;\n"
											 , &error);
								 }
							}
							else
							{
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
										/* RGBA texture */
										append_string(&fragment_program_string,
											 "MOV		finalCol, tex;\n"
											 , &error);
								 }
								 else
								 {
										append_string(&fragment_program_string,
											 "  color = tex;\n"
											 , &error);
								 }
							}
						}
					}
					if ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1 |
							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2 |
							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3 |
							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4) & shader_program->type)
					{
						if ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR |

							SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA) & shader_program->type)
						{
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "TEMP dependentlookup;\n"
										 "TEMP offsetcolour;\n"
										 , &error);
							 }
							 components_string = (char *)NULL;
							 components_error = 0;
							 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1
									& shader_program->type)
							 {
									append_string(&components_string, "r", &components_error);
							 }
							 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2
									& shader_program->type)
							 {
									append_string(&components_string, "g", &components_error);
							 }
							 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3
									& shader_program->type)
							 {
									append_string(&components_string, "b", &components_error);
							 }
							 if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4
									& shader_program->type)
							 {
									append_string(&components_string, "a", &components_error);
							 }
							 number_of_inputs = static_cast<int>(strlen(components_string));
							 while (!components_error && (strlen(components_string) < 4))
							 {
								append_string(&components_string, "r", &components_error);
							 }
							 if (!components_error)
							 {
									char tex_string[1000];
									if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
									{
										 sprintf(tex_string,
												"#Offset and scale to counteract effect of linear interpolation\n"
												"#starting at the middle of the first texel and finishing in the\n"
												"#middle of the last texel\n"
												"MAD		offsetcolour, finalCol.%s, lookup_scales, lookup_offsets;\n"
												"TEX		dependentlookup, offsetcolour, texture[1], %1dD;\n",
												components_string, number_of_inputs);
									}
									else
									{
										 if (number_of_inputs == 1)
										 {
												sprintf(tex_string,
													 "  //Offset and scale to counteract effect of linear interpolation\n"
													 "  //starting at the middle of the first texel and finishing in the\n"
													 "  //middle of the last texel\n"
													 "  vec4  offsetcolour = color.%s * lookup_scales + lookup_offsets;\n"
													 "  vec4  dependentlookup = texture1D(texture1, float(offsetcolour));\n",
													 components_string);
										 }
										 else
										 {
												sprintf(tex_string,
													 "  //Offset and scale to counteract effect of linear interpolation\n"
													 "  //starting at the middle of the first texel and finishing in the\n"
													 "  //middle of the last texel\n"
													 "  vec4  offsetcolour = color.%s * lookup_scales + lookup_offsets;\n"
													 "  vec4  dependentlookup = texture%1dD(texture1, vec%1d(offsetcolour));\n",
													 components_string, number_of_inputs, number_of_inputs);
										 }
									}
									append_string(&fragment_program_string,
										 tex_string, &error);
							}
							if (components_string)
							{
								DEALLOCATE(components_string);

							}
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 switch ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR |
											 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA)
										& shader_program->type)
								 {
										case SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR:
										{
											 /* Don't touch alpha */
											 append_string(&fragment_program_string,
													"MOV		finalCol.rgb, dependentlookup;\n"
													, &error);
										} break;
										case SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA:
										{
											 append_string(&fragment_program_string,
													"MUL		finalCol.w, finalCol.w, dependentlookup.r;\n"
													, &error);
										} break;
										case (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR |
											 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA):
											 {
													append_string(&fragment_program_string,
														 "MOV		finalCol, dependentlookup;\n"
														 , &error);
											 } break;
								 }
							}
							else
							{
								 switch ((SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR |
											 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA)
										& shader_program->type)
								 {
										case SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR:
										{
											 /* Don't touch alpha */
											 append_string(&fragment_program_string,
													" 	color.rgb = dependentlookup.rgb;\n"
													, &error);
										} break;
										case SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA:
										{
											 append_string(&fragment_program_string,
													"  color.w = color.w * dependentlookup.r;\n"
													, &error);
										} break;
										case (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_COLOUR |
											 SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_ALPHA):
											 {
													append_string(&fragment_program_string,
														 "  color = dependentlookup;\n"
														 , &error);
											 } break;

								 }
							}
						}
						else if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1D_COMPONENT_LOOKUP & shader_program->type)
						{
							 char tex_string[1000];
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									char lookup_one_component_string[] =
										 "#Offset and scale to counteract effect of linear interpolation\n"
										 "#starting at the middle of the first texel and finishing in the\n"
										 "#middle of the last texel\n"
										 "MAD		offsetcolour, finalCol.%s, lookup_scales, lookup_offsets;\n"
										 "TEX		dependentlookup, offsetcolour, texture[1], 1D;\n"
										 "MOV		finalCol.%s, dependentlookup.r;\n";
									append_string(&fragment_program_string,
										 "TEMP dependentlookup;\n"
										 "TEMP offsetcolour;\n"
										 , &error);
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"rrrr", "r");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"gggg", "g");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"bbbb", "b");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"aaaa", "a");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
							 }
							 else
							 {
								  append_string(&fragment_program_string,
									   "  //Offset and scale to counteract effect of linear interpolation\n"
									   "  //starting at the middle of the first texel and finishing in the\n"
									   "  //middle of the last texel\n"
									   "  float  offsetcolour;\n"
										 "  vec4  dependentlookup;\n", &error);
									char lookup_one_component_string[] =
										 "  offsetcolour = color.%s * lookup_scales.x + lookup_offsets.x;\n"
										 "  dependentlookup = texture1D(texture1, offsetcolour);\n"
										 "  color.%s = dependentlookup.r;\n";
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_1
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"r", "r");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_2
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"g", "g");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_3
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"b", "b");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
									if (SHADER_PROGRAM_CLASS_DEPENDENT_TEXTURE_4
										 & shader_program->type)
									{
										 sprintf(tex_string, lookup_one_component_string,
												"a", "a");
										 append_string(&fragment_program_string,
												tex_string, &error);
									}
							 }
						}
					}

					if ((SHADER_PROGRAM_CLASS_LIT_VOLUME_INTENSITY_NORMAL_TEXTURE |
						SHADER_PROGRAM_CLASS_LIT_VOLUME_FINITE_DIFFERENCE_NORMAL) &
						shader_program->type)
					{
						/* I think with some rearrangement we could consolidate
							this with the per pixel lighting above assuming that we
							don't want to light using the fragment normals and
							then do this lighting too. */
						 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
						 {
								append_string(&fragment_program_string,
									 "TEMP unlitColour;\n"
									 "MOV     unlitColour, finalCol;\n"
									 , &error);
						 }
						 else
						 {
								append_string(&fragment_program_string,
									 "  vec4 unlitColour = color;\n"
									 , &error);
						 }

						if (SHADER_PROGRAM_CLASS_LIT_VOLUME_INTENSITY_NORMAL_TEXTURE &
							shader_program->type)
						{
							 /* Normal comes from the texture */

							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "#Expand the range of the normal texture\n"
										 /* We are assuming the normal is in .gba */
										 "MAD     normal, two, tex.gbaa, m_one;\n"
										 , &error);
							 }
							 else
							 {
									append_string(&fragment_program_string,
										 "  n = 2 * tex.gbaa - 1.0;\n"
										 , &error);
							 }
						}
						else
						{
							char tex_string[1000];
							const char *component_labels[] = {"x", "y", "z"};
							int i;

							/* Normal is calculated from the red intensity,
								may want colour magnitude or alpha value. */
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&fragment_program_string,
										"#Calculate a finite difference normal based on the magnitude of texture components used.\n"
										"PARAM texture_scaling = program.env[0];\n"
										"PARAM normal_scaling = program.env[3];\n"
										"TEMP position_up, position_down, tex_up, tex_down;\n"
										"\n"
										, &error);
							}
							else
							{
								 append_string(&fragment_program_string,
										"//Calculate a finite difference normal based on the magnitude of texture components used.\n"
										"  vec4 position_up, position_down, tex_up, tex_down;\n"
										, &error);
							}

							if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_1 & shader_program->type)
							{
								if (SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_2 & shader_program->type)
								{
									colour_texture_dimension = 3;
								}
								else
								{
									colour_texture_dimension = 1;
								}

							}
							else
							{
								colour_texture_dimension = 2;
							}

							for (i = 0 ; i < colour_texture_dimension ; i++)
							{
								 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
								 {
									 sprintf(tex_string,
											 "PARAM stencil_%sup = {%d, %d, %d, %d};\n"
											 "MAD      position_up, stencil_%sup, texture_scaling, fragment.texcoord[0];\n"
											 "TEX		tex_up, position_up, texture[0], %dD;\n"
											 "PARAM stencil_%sdown = {%d, %d, %d, %d};\n"
											 "MAD      position_down, stencil_%sdown, texture_scaling, fragment.texcoord[0];\n"
											 "TEX		tex_down, position_down, texture[0], %dD;\n",
											 component_labels[i],
											 (i==0),(i==1),(i==2),0,
											 component_labels[i],
											 colour_texture_dimension,
											 component_labels[i],
											 -(i==0),-(i==1),-(i==2),0,
											 component_labels[i],
											 colour_texture_dimension);
										append_string(&fragment_program_string,
											 tex_string, &error);

										switch ((SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1
													| SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2) & shader_program->type)
										{
											 case SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1
													| SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2:
											 case 0:
											 {
													/* RGB or RGBA texture
														 Take the magnitude of the differences */
													sprintf(tex_string,
														 "SUB  tex_up, tex_up, tex_down;\n"
														 "DP3	tex_up.w, tex_up, tex_up;\n"
														 "RSQ  tex_up.w, tex_up.w;\n"
														 "RCP  normal.%s, tex_up.w;\n"
														 , component_labels[i]);
													append_string(&fragment_program_string,
														 tex_string, &error);
											 } break;
											 case SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1:
											 case SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2:
											 {
													/* Intensity or IntensityAlpha texture */
													sprintf(tex_string,
														 "SUB  normal.%s, tex_up.r, tex_down.r;\n"
														 , component_labels[i]);
													append_string(&fragment_program_string,
														 tex_string, &error);
											 } break;
										}
								 }
								 else
								 {
										sprintf(tex_string,
											 "  vec4 stencil_%sup = vec4(%d, %d, %d, %d);\n"
											 "  position_up = stencil_%sup * texture_scaling + gl_TexCoord[0];\n"
											 "  tex_up = texture%dD(texture0, vec%d(position_up));\n"
											 "  vec4 stencil_%sdown = vec4(%d, %d, %d, %d);\n"
											 "  position_down = stencil_%sdown * texture_scaling + gl_TexCoord[0];\n"
											 "  tex_down =  texture%dD(texture0, vec%d(position_down));\n",
											 component_labels[i],
											 (i==0),(i==1),(i==2),0,
											 component_labels[i],
											 colour_texture_dimension,
											 colour_texture_dimension,
											 component_labels[i],
											 -(i==0),-(i==1),-(i==2),0,
											 component_labels[i],
											 colour_texture_dimension,
											 colour_texture_dimension);

										append_string(&fragment_program_string,
											 tex_string, &error);

										switch ((SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1
													| SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2) & shader_program->type)
										{
											 case SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1
													| SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2:
											 case 0:
											 {
													/* RGB or RGBA texture
														 Take the magnitude of the differences */
													sprintf(tex_string,
														 "  n.%s = sqrt(dot(vec3(tex_up - tex_down), vec3(tex_up - tex_down)));\n"
														 , component_labels[i]);
													append_string(&fragment_program_string,
														 tex_string, &error);
											 } break;
											 case SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_1:
											 case SHADER_PROGRAM_CLASS_COLOUR_TEXTURE_OUTPUT_2:
											 {
													/* Intensity or IntensityAlpha texture */
													sprintf(tex_string,
														 "  n.%s = tex_up.r - tex_down.r;\n"
														 , component_labels[i]);
													append_string(&fragment_program_string,
														 tex_string, &error);
											 } break;
										}
								 }
							}
							if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							{
								 append_string(&fragment_program_string,
										"MUL  normal, normal, normal_scaling;\n"
										, &error);
							}
							else
							{
								 append_string(&fragment_program_string,
										"  n = n * vec3(normal_scaling);\n"
										, &error);
							}
						}

						if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
						{
							 append_string(&fragment_program_string,
									/* Normalise the normal but keep the squared
										 magnitude so we can use it to scale the alpha */
									"TEMP  eyeNormal, normalMag;\n"
									"PARAM c2[4] = { state.matrix.modelview.invtrans };\n"
									"\n"
									"#Put the normal into eye point space\n"
									"DP4 eyeNormal.x, c2[0], normal;\n"
									"DP4 eyeNormal.y, c2[1], normal;\n"
									"DP4 eyeNormal.z, c2[2], normal;\n"
									"\n"
									"#Reverse the texture normal direction component if required\n"
									"MUL      eyeNormal, reverse, eyeNormal;\n"
									"\n"
									"#Normalize the normal.\n"
									"DP3		normalMag.w, eyeNormal, eyeNormal;\n"
									"RSQ		eyeNormal.w, normalMag.w;\n"
									"MUL		eyeNormal.xyz, eyeNormal, eyeNormal.w;\n"
									"\n"
									"#Normalize lightvec and viewvec.\n"
									"DP3		Len.w, fragment.texcoord[1], fragment.texcoord[1];\n"
									"RSQ		lightVec.w, Len.w;\n"
									"MUL		lightVec.xyz, fragment.texcoord[1], lightVec.w;\n"
									"DP3		viewVec.w, fragment.texcoord[2], fragment.texcoord[2];\n"
									"RSQ		viewVec.w, viewVec.w;\n"
									"MUL		viewVec.xyz, fragment.texcoord[2], viewVec.w;\n"
									"#Calculate attenuation.\n"
									"MAD		attenuation, state.light[0].attenuation.z, Len.w, state.light[0].attenuation.x;\n"
									"RCP		Len, lightVec.w;\n"
									"MAD		attenuation, Len.w, state.light[0].attenuation.y, attenuation.x;\n"
									"RCP		attenuation.x, attenuation.x;\n"
									"#Diffuse\n"
									"DP3_SAT	   lightContrib.x, eyeNormal, lightVec;\n"
									""
									"#Specular\n"
									"# Phong:\n"
									"DP3		reflVec, lightVec, eyeNormal;\n"
									"MUL		reflVec, reflVec, two;\n"
									"MAD		reflVec, reflVec, -eyeNormal, lightVec;\n"
									"\n"
									"DP3_SAT	lightContrib.y, reflVec, viewVec;\n"
									"MOV		lightContrib.w, state.material.shininess.x;\n"
									"#Accelerates lighting computations\n"
									"LIT	   lightContrib, lightContrib;\n"
									"MAD		finalCol, lightContrib.y, unlitColour, state.lightprod[0].ambient;\n"
									"MAD		finalCol, lightContrib.z, state.lightprod[0].specular, finalCol;\n"
									"MAD		finalCol, finalCol, attenuation.x, state.material.emission;\n"
									"#Ambient lighting contribution;\n"
									"MAD		finalCol, unlitColour, state.lightmodel.ambient, finalCol;\n"
									, &error);
						}
						else
						{
							 append_string(&fragment_program_string,
									/* Normalise the normal but keep the squared
										 magnitude so we can use it to scale the alpha */
									"  vec3 eyeNormal = gl_NormalMatrix * n;\n"
									"  if (!gl_FrontFacing)\n"
									"    eyeNormal.z = -1.0 * eyeNormal.z;\n"
									"  float normalMag = dot (eyeNormal, eyeNormal);\n"
									"  eyeNormal = normalize(eyeNormal);\n"
									"  len = length(vec3(gl_TexCoord[1]));\n"
									"  att = 1.0 / (gl_LightSource[0].constantAttenuation +\n"
									"    gl_LightSource[0].linearAttenuation * len +\n"
									"    gl_LightSource[0].quadraticAttenuation * len * len);\n"
									"  //Calculate attenuation.\n"
									"  NdotL = (dot(eyeNormal, normalize(gl_TexCoord[1].xyz)));\n"
									"  if (!gl_FrontFacing)\n"
									"    NdotL = abs(NdotL);\n"
									"  color += att * (diffuse *NdotL + ambient);\n"
									"  reflV = reflect(-normalize(gl_TexCoord[1].xyz), eyeNormal);\n"
									"  NdotHV = max(dot(reflV, normalize(gl_TexCoord[2].xyz)),0.0);\n"
									"  color += att * gl_FrontMaterial.specular * gl_LightSource[0].specular *\n"
									"    pow(NdotHV, gl_FrontMaterial.shininess);\n"
									, &error);
						}

						if (SHADER_PROGRAM_CLASS_LIT_VOLUME_SCALE_ALPHA &
							shader_program->type)
						{
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "#Alpha value;\n"
										 "MUL		finalCol.w, unlitColour.w, normalMag.w;\n"
										 , &error);
							 }
							 else
							 {
									append_string(&fragment_program_string,
										 "  //Alpha value;\n"
										 "  color.w = unlitColour.w * normalMag;\n"
										 , &error);
							 }
						}
						else
						{
							 if (shader_program->shader_type!=SHADER_PROGRAM_SHADER_GLSL)
							 {
									append_string(&fragment_program_string,
										 "#Alpha value;\n"
										 "MOV		finalCol.w, unlitColour.w;\n"
										 , &error);
							 }
							 else
							 {
									append_string(&fragment_program_string,
										 "//Alpha value;\n"
										 "	color.w = unlitColour.w;\n"
										 , &error);
							 }
						}
					}
					if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					{
						 append_string(&fragment_program_string,
								"  gl_FragColor = color;\n"
								"}"
								, &error);

					}
					else


					{
						 append_string(&fragment_program_string,
								"MOV		result.color, finalCol;\n"
								"END"
								, &error);
					}
				}
				if (vertex_program_string && fragment_program_string)
				{
					if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					{
#if defined (WRITE_STRING)
						FILE *program_file;
						if (vertex_program_string && (program_file = fopen("out.vert", "w")))
						{
							fprintf(program_file, "%s", vertex_program_string);
							fclose (program_file);
						}
						if (fragment_program_string && (program_file = fopen("out.frag", "w")))
						{
							fprintf(program_file, "%s", fragment_program_string);
							fclose (program_file);
						}
						if (geometry_program_string && (program_file = fopen("out.geo", "w")))
						{
							fprintf(program_file, "%s", geometry_program_string);
							fclose (program_file);
						}
#endif /* defined (WRITE_STRING) */
#if defined (GL_VERSION_2_0)
						shader_program->vertex_program = glCreateShader(GL_VERTEX_SHADER);
						shader_program->fragment_program = glCreateShader(GL_FRAGMENT_SHADER);
						if (geometry_program_string && Graphics_library_load_extension("GL_EXT_geometry_shader4"))
						{
							shader_program->geometry_program = glCreateShader(GL_GEOMETRY_SHADER_EXT);
						}
#endif /* defined (GL_VERSION_2_0) */
					}
					else
					{
						if (!shader_program->vertex_program)
						{
							glGenProgramsARB(1, &shader_program->vertex_program);
						}
						glBindProgramARB(GL_VERTEX_PROGRAM_ARB, shader_program->vertex_program);
						glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
							static_cast<GLsizei>(strlen(vertex_program_string)), vertex_program_string);
#if defined (DEBUG_CODE)
						error_msg = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
						display_message(WARNING_MESSAGE,
							"cmzn_shaderprogram_compile.  Vertex Result: %s\n", error_msg);
#endif /* defined (DEBUG_CODE) */
#if defined (WRITE_STRING)
						FILE *program_file;
						if (vertex_program_string && (program_file = fopen("out.vp", "w")))
						{
							fprintf(program_file, "%s", vertex_program_string);
							fclose (program_file);
						}
#endif /* defined (WRITE_STRING) */
						DEALLOCATE(vertex_program_string);
						if (!shader_program->fragment_program)
						{
							glGenProgramsARB(1, &shader_program->fragment_program);
						}
						glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_program->fragment_program);
						glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
							static_cast<GLsizei>(strlen(fragment_program_string)), fragment_program_string);
#if defined (DEBUG_CODE)
						error_msg = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
						display_message(WARNING_MESSAGE,
							"cmzn_shaderprogram_compile.  Fragment Result: %s\n", error_msg);
#endif /* defined (DEBUG_CODE) */
#if defined (WRITE_STRING)
						if (fragment_program_string && (program_file = fopen("out.fp", "w")))
						{
							fprintf(program_file, "%s", fragment_program_string);
							fclose (program_file);
						}
#endif /* defined (WRITE_STRING) */
						DEALLOCATE(fragment_program_string);
					}
#else /* ! defined (TESTING_PROGRAM_STRINGS) */
#define MAX_PROGRAM (20000)
					char vertex_program_string[MAX_PROGRAM], fragment_program_string[MAX_PROGRAM];
					{
						FILE *program_file;
						int count;

						if (program_file = fopen("test.vp", "r"))
						{
							count = fread(vertex_program_string, 1, MAX_PROGRAM - 1, program_file);
							vertex_program_string[count] = 0;
							if (count > MAX_PROGRAM - 2)
							{
								display_message(ERROR_MESSAGE, "cmzn_shaderprogram_compile.  "
									"Short read on test.vp, need to increase MAX_PROGRAM.");
							}
							fclose (program_file);
						}
						else
						{
							display_message(ERROR_MESSAGE, "cmzn_shaderprogram_compile.  "
								"Unable to open file test.vp.");
						}

						if (program_file = fopen("test.fp", "r"))
						{
							count = fread(fragment_program_string, 1, MAX_PROGRAM - 1, program_file);
							fragment_program_string[count] = 0;
							if (count > MAX_PROGRAM - 2)
							{
								display_message(ERROR_MESSAGE, "cmzn_shaderprogram_compile.  "
									"Short read on test.fp, need to increase MAX_PROGRAM.");
							}
							fclose (program_file);
						}
						else
						{
							display_message(ERROR_MESSAGE, "cmzn_shaderprogram_compile.  "
								"Unable to open file test.fp.");
						}
					}
					if (shader_program->shader_type == SHADER_PROGRAM_SHADER_GLSL)
					{
#if defined (GL_VERSION_2_0)
						shader_program->vertex_program = glCreateShader(GL_VERTEX_SHADER);
						shader_program->fragment_program = glCreateShader(GL_FRAGMENT_SHADER);
						if (geometry_program_string && Graphics_library_load_extension("GL_EXT_geometry_shader4"))
						{
							shader_program->geometry_program = glCreateShader(GL_GEOMETRY_SHADER_EXT);
						}
#endif /* defined (GL_VERSION_2_0) */
					}
					else
					{
						if (!shader_program->vertex_program)
						{
							glGenProgramsARB(1, &shader_program->vertex_program);
						}

						glBindProgramARB(GL_VERTEX_PROGRAM_ARB, shader_program->vertex_program);
						glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
							strlen(vertex_program_string), vertex_program_string);
#if defined (DEBUG_CODE)
						error_msg = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
						display_message(WARNING_MESSAGE,
							"cmzn_shaderprogram_compile.  test.vp Vertex Result: %s", error_msg);
#endif /* defined (DEBUG_CODE) */

						if (!shader_program->fragment_program)
						{
							glGenProgramsARB(1, &shader_program->fragment_program);
						}

						glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader_program->fragment_program);
						glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
							strlen(fragment_program_string), fragment_program_string);
#if defined (DEBUG_CODE)
						error_msg = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
						display_message(WARNING_MESSAGE,
							"cmzn_shaderprogram_compile.  test.fp Fragment Result: %s", error_msg);
#endif /* defined (DEBUG_CODE) */
					}
					shader_program->compiled = 1;
#endif /* ! defined (TESTING_PROGRAM_STRINGS) */
					if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
					{
						GLint vertexShaderCompiled, fragmentShaderCompiled, geometryShaderCompiled;

						shader_program->glsl_current_program = glCreateProgram();

						vv = vertex_program_string;
						glShaderSource(shader_program->vertex_program,1, &vv, NULL);
						glCompileShader(shader_program->vertex_program);
						glGetShaderiv(shader_program->vertex_program, GL_COMPILE_STATUS, &vertexShaderCompiled);
						glAttachShader(shader_program->glsl_current_program,shader_program->vertex_program);
						DEALLOCATE(vertex_program_string);
						if (shader_program->geometry_program)
						{
							gg = geometry_program_string;
							glShaderSource(shader_program->geometry_program,1, &gg, NULL);
							glCompileShader(shader_program->geometry_program);
							glProgramParameteriEXT(shader_program->glsl_current_program, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
							glProgramParameteriEXT(shader_program->glsl_current_program, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
							glGetShaderiv(shader_program->geometry_program, GL_COMPILE_STATUS, &geometryShaderCompiled);
							glAttachShader(shader_program->glsl_current_program,shader_program->geometry_program);
							int geom_ouput_max_vertices = 0;
							glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, &geom_ouput_max_vertices);
							glProgramParameteriEXT(shader_program->glsl_current_program, GL_GEOMETRY_VERTICES_OUT_EXT, geom_ouput_max_vertices);
							DEALLOCATE(geometry_program_string);
						}
						else
						{
							geometryShaderCompiled = 1;
						}

						ff = fragment_program_string;
						glShaderSource(shader_program->fragment_program,1, &ff, NULL);
						glCompileShader(shader_program->fragment_program);
						glGetShaderiv(shader_program->fragment_program, GL_COMPILE_STATUS, &fragmentShaderCompiled);
						glAttachShader(shader_program->glsl_current_program,shader_program->fragment_program);
						DEALLOCATE(fragment_program_string);

#if !defined (DEBUG_CODE)
						// If DEBUG_CODE is defined always write the program info, otherwise
						// write the program info only when one of the shaders fails to compile.
						if (!vertexShaderCompiled || !fragmentShaderCompiled || !geometryShaderCompiled)
#endif // !defined (DEBUG_CODE)
						{
							int infologLength = 0;
							int charsWritten  = 0;
							char *infoLog;
							glGetShaderiv(shader_program->vertex_program, GL_INFO_LOG_LENGTH,&infologLength);
							if (infologLength > 0)
							{
								infoLog = (char *)malloc(infologLength);
								glGetShaderInfoLog(shader_program->vertex_program,
									infologLength, &charsWritten, infoLog);
								display_message(INFORMATION_MESSAGE,"Vertex program info:\n%s\n",infoLog);
								free(infoLog);
							}
							if (shader_program->geometry_program)
							{
								glGetShaderiv(shader_program->geometry_program, GL_INFO_LOG_LENGTH,&infologLength);
								if (infologLength > 0)
								{
									infoLog = (char *)malloc(infologLength);
									glGetShaderInfoLog(shader_program->geometry_program,
										infologLength, &charsWritten, infoLog);
									display_message(INFORMATION_MESSAGE,"Geometry program info:\n%s\n",infoLog);
									free(infoLog);
								}
							}
							glGetShaderiv(shader_program->fragment_program, GL_INFO_LOG_LENGTH,&infologLength);
							if (infologLength > 0)
							{
								infoLog = (char *)malloc(infologLength);
								glGetShaderInfoLog(shader_program->fragment_program,
									infologLength, &charsWritten, infoLog);
								display_message(INFORMATION_MESSAGE,"Fragment program info:\n%s\n",infoLog);
								free(infoLog);
							}
						}
						if (renderer->use_display_list)
						{
							if (!shader_program->display_list)
							{
								shader_program->display_list = glGenLists(1);
							}
							glNewList(shader_program->display_list, GL_COMPILE);
							glLinkProgram(shader_program->glsl_current_program);
							glUseProgram(shader_program->glsl_current_program);
							glEnable(GL_VERTEX_PROGRAM_TWO_SIDE);
							glEndList();
						}
					}
					else
					{
						if (renderer->use_display_list)
						{
							if (!shader_program->display_list)
							{
								shader_program->display_list = glGenLists(/*number_of_lists*/1);
							}

							glNewList(shader_program->display_list, GL_COMPILE);

							glEnable(GL_VERTEX_PROGRAM_ARB);
							glBindProgramARB(GL_VERTEX_PROGRAM_ARB,
									shader_program->vertex_program);

							glEnable(GL_FRAGMENT_PROGRAM_ARB);
							glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
									shader_program->fragment_program);

							glEnable(GL_VERTEX_PROGRAM_TWO_SIDE_ARB);

							glEndList();
						}
					}
				}
				shader_program->compiled = 1;
			 }
			 else
			 {
				 display_message(ERROR_MESSAGE, "Support for per pixel lighting and "
					 "bump mapping requires the "
					 "GL_ARB_vertex_program and GL_ARB_fragment_program extensions "
					 "which are not available in this OpenGL implementation.");
			 }
#else /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
			 display_message(ERROR_MESSAGE, "Support for per pixel lighting and "
				 "bump mapping requires the "
				 "GL_ARB_vertex_program and GL_ARB_fragment_program extensions or GL_VERSION_2_0 "
				 "which were not compiled into this version.");
#endif /* defined GL_ARB_vertex_program && defined GL_ARB_fragment_program */
		}
#else /* defined (OPENGL_API) */
		display_message(ERROR_MESSAGE,
			"cmzn_shaderprogram_compile.  Not defined for this graphics API.");
		return_code=0;
#endif /* defined (OPENGL_API) */
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_shaderprogram_compile.  Missing shader_program");
		return_code=0;
	}
	LEAVE;

	return (return_code);
} /* cmzn_shaderprogram_compile */

int cmzn_shaderprogram_execute(cmzn_shaderprogram_id shader_program,
	Render_graphics_opengl *renderer)
/*******************************************************************************
LAST MODIFIED : 20 November 2003

DESCRIPTION :
==============================================================================*/
{
	int return_code;

	if (shader_program)
	{
		if (shader_program->compiled)
		{
			if (renderer->use_display_list)
			{
				if (shader_program->display_list)
				{
					glCallList(shader_program->display_list);
				}
			}
			else
			{
				if (shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL)
				{
					if (shader_program->glsl_current_program)
					{
						GLint linked= 0;
						glGetProgramiv(shader_program->glsl_current_program, GL_LINK_STATUS, &linked);
						if (linked)
						{
							glUseProgram(shader_program->glsl_current_program);
						}
						else
						{
							glLinkProgram(shader_program->glsl_current_program);
							glUseProgram(shader_program->glsl_current_program);
						}
						glEnable(GL_VERTEX_PROGRAM_TWO_SIDE);
					}
				}
				else
				{
					if (shader_program->vertex_program && shader_program->fragment_program)
					{
						glEnable(GL_VERTEX_PROGRAM_ARB);
						glBindProgramARB(GL_VERTEX_PROGRAM_ARB,
								shader_program->vertex_program);

						glEnable(GL_FRAGMENT_PROGRAM_ARB);
						glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB,
								shader_program->fragment_program);
						glEnable(GL_VERTEX_PROGRAM_TWO_SIDE_ARB);
					}
				}
			}
			return_code = 1;
		}
		else
		{
			return_code = 0;
		}
	}
	else
	{
		display_message(ERROR_MESSAGE,
			"cmzn_shaderprogram_execute.  Missing shader_program object.");
		return_code = 0;
	}

	return (return_code);
} /* cmzn_shaderprogram_execute */

int cmzn_shaderprogram_execute_textures(cmzn_shaderprogram_id shader_program,
	struct Texture *texture, struct Texture *second_texture,
	struct Texture *third_texture)
{
#if defined (GL_VERSION_2_0)
	if (shader_program && shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL &&
			shader_program->glsl_current_program)
	{
		 if (texture)
		 {
			Texture_execute_vertex_program_environment(texture,
					shader_program->glsl_current_program);
		 }
		 if (second_texture)
		 {
			Texture_execute_vertex_program_environment(second_texture,
					shader_program->glsl_current_program);

		 }
		 if (third_texture)
		 {
			Texture_execute_vertex_program_environment(third_texture,
				 shader_program->glsl_current_program);
		 }
		 return 1;
	}
#endif
	return 0;
}

int cmzn_shaderprogram_execute_uniforms(cmzn_shaderprogram_id shader_program,
	cmzn_shaderuniforms_id uniforms)
{
#if defined (GL_VERSION_2_0)
	if (shader_program && shader_program->shader_type==SHADER_PROGRAM_SHADER_GLSL &&
			shader_program->glsl_current_program)
	{
	 if (glIsProgram(shader_program->glsl_current_program))
	 {
		GLint loc1 = -1;
		loc1 = glGetUniformLocation(shader_program->glsl_current_program,"texture2");
		if (loc1 != (GLint)-1)
			 glUniform1i(loc1,2);
		loc1 = glGetUniformLocation(shader_program->glsl_current_program,"texture1");
		if (loc1 != (GLint)-1)
			 glUniform1i(loc1,1);
		loc1 = glGetUniformLocation(shader_program->glsl_current_program,"texture0");
		if (loc1 != (GLint)-1)
			 glUniform1i(loc1, 0);
		if (uniforms)
			cmzn_shaderuniforms_write_to_shaders(uniforms, shader_program->glsl_current_program);;
		return 1;
	 }

	}
#endif
	return 0;
}

#endif /* defined (OPENGL_API) */

int cmzn_shaderprogram_manager_set_owner_private(struct MANAGER(cmzn_shaderprogram) *manager,
	struct cmzn_shadermodule *shadermodule)
{
	return MANAGER_SET_OWNER(cmzn_shaderprogram)(manager, shadermodule);
}

int cmzn_shaderprogram_manager_message_get_object_change_and_detail(
	struct MANAGER_MESSAGE(cmzn_shaderprogram) *message, cmzn_shaderprogram *shaderprogram,
	const cmzn_shaderprogram_change_detail **change_detail_address)
{
	if (message)
		return message->getObjectChangeFlagsAndDetail(shaderprogram, change_detail_address);
	if (change_detail_address)
		*change_detail_address = 0;
	return (MANAGER_CHANGE_NONE(cmzn_shaderprogram));
}

cmzn_shaderprogram_id cmzn_shaderprogram_create_private()
{
	return cmzn_shaderprogram::create();
}


cmzn_shaderprogram_id cmzn_shaderprogram_access(cmzn_shaderprogram_id shader_program)
{
	if (shader_program)
		return ACCESS(cmzn_shaderprogram)(shader_program);
	return 0;
}

int cmzn_shaderprogram_destroy(cmzn_shaderprogram_id *shader_program_address)
{
	if (shader_program_address && *shader_program_address)
	{
		DEACCESS(cmzn_shaderprogram)(shader_program_address);
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

bool cmzn_shaderprogram_is_managed(cmzn_shaderprogram_id shaderprogram)
{
	if (shaderprogram)
	{
		return shaderprogram->is_managed_flag;
	}
	return 0;
}

int cmzn_shaderprogram_set_managed(cmzn_shaderprogram_id shaderprogram,
	bool value)
{
	if (shaderprogram)
	{
		bool old_value = shaderprogram->is_managed_flag;
		shaderprogram->is_managed_flag = (value != 0);
		if (value != old_value)
		{
			MANAGED_OBJECT_CHANGE(cmzn_shaderprogram)(shaderprogram,
				MANAGER_CHANGE_DEFINITION(cmzn_shaderprogram));
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_shaderprogram_get_vertex_shader(cmzn_shaderprogram_id program)
{
	if (program && program->vertex_program_string)
	{
		return duplicate_string(program->vertex_program_string);
	}

	return 0;
}

char *cmzn_shaderprogram_get_fragment_shader(cmzn_shaderprogram_id program)
{
	if (program && program->fragment_program_string)
	{
		return duplicate_string(program->fragment_program_string);
	}

	return 0;
}

char *cmzn_shaderprogram_get_geometry_shader(cmzn_shaderprogram_id program)
{
	if (program && program->geometry_program_string)
	{
		return duplicate_string(program->geometry_program_string);
	}

	return 0;
}

int cmzn_shaderprogram_set_vertex_shader(cmzn_shaderprogram_id program,
	const char *vertex_shader_string)
{
	if (program && vertex_shader_string)
	{
		if (program->vertex_program_string)
		{
			DEALLOCATE(program->vertex_program_string);
		}
		program->vertex_program_string = duplicate_string(vertex_shader_string);
		program->programChanged();
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderprogram_set_fragment_shader(cmzn_shaderprogram_id program,
	const char *fragment_shader_string)
{
	if (program && fragment_shader_string)
	{
		if (program->fragment_program_string)
		{
			DEALLOCATE(program->fragment_program_string);
		}
		program->fragment_program_string = duplicate_string(fragment_shader_string);
		program->programChanged();
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

int cmzn_shaderprogram_set_geometry_shader(cmzn_shaderprogram_id program,
	const char *geometry_shader_string)
{
	if (program && geometry_shader_string)
	{
		if (program->geometry_program_string)
		{
			DEALLOCATE(program->geometry_program_string);
		}
		program->geometry_program_string = duplicate_string(geometry_shader_string);
		program->programChanged();
		return CMZN_OK;
	}

	return CMZN_ERROR_ARGUMENT;
}

unsigned int cmzn_shaderprogram_get_glslprogram(cmzn_shaderprogram_id program)
{
#if defined (OPENGL_API)
	if (program)
	{
		return (unsigned int)program->glsl_current_program;
	}
#endif
	return 0;
}

enum cmzn_shaderprogram_shader_type cmzn_shaderprogram_get_shader_type(cmzn_shaderprogram_id program)
{
#if defined (OPENGL_API)
	if (program)
	{
		return program->shader_type;
	}
#endif
	return SHADER_PROGRAM_SHADER_NONE;
}

enum cmzn_shaderprogram_type cmzn_shaderprogram_get_type(cmzn_shaderprogram_id program)
{
	if (program)
	{
		return program->type;
	}
	return 	SHADER_PROGRAM_SPECIFIED_STRINGS;
}

int cmzn_shaderprogram_set_type(cmzn_shaderprogram_id shaderprogram, enum cmzn_shaderprogram_type type)
{
	if (shaderprogram)
	{
		shaderprogram->type = type;
		shaderprogram->compiled = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

char *cmzn_shaderprogram_get_name(cmzn_shaderprogram_id shaderprogram)
{
	char *name = NULL;
	if (shaderprogram && shaderprogram->name)
	{
		name = duplicate_string(shaderprogram->name);
	}
	return name;
}

int cmzn_shaderprogram_set_name(cmzn_shaderprogram_id shaderprogram, const char *name)
{
	int return_code;

	if (shaderprogram && name)
	{
		return_code = 1;
		cmzn_set_cmzn_shaderprogram *manager_shaderprogram_list = 0;
		bool restore_changed_object_to_lists = false;
		if (shaderprogram->manager)
		{
			cmzn_shaderprogram *existing_shaderprogram =
				FIND_BY_IDENTIFIER_IN_MANAGER(cmzn_shaderprogram, name)(name, shaderprogram->manager);
			if (existing_shaderprogram && (existing_shaderprogram != shaderprogram))
			{
				display_message(ERROR_MESSAGE, "cmzn_shaderprogram_set_name.  "
					"shaderprogram named '%s' already exists.", name);
				return_code = 0;
			}
			if (return_code)
			{
				manager_shaderprogram_list = reinterpret_cast<cmzn_set_cmzn_shaderprogram *>(
					shaderprogram->manager->object_list);
				// this temporarily removes the object from all related lists
				restore_changed_object_to_lists =
					manager_shaderprogram_list->begin_identifier_change(shaderprogram);
				if (!restore_changed_object_to_lists)
				{
					display_message(ERROR_MESSAGE, "cmzn_shaderprogram_set_name.  "
						"Could not safely change identifier in manager");
					return_code = 0;
				}
			}
		}
		if (return_code)
		{
			char *new_name = duplicate_string(name);
			if (new_name)
			{
				DEALLOCATE(shaderprogram->name);
				shaderprogram->name = new_name;
			}
			else
			{
				return_code = 0;
			}
		}
		if (restore_changed_object_to_lists)
		{
			manager_shaderprogram_list->end_identifier_change();
		}
		if (shaderprogram->manager && return_code)
		{
			MANAGED_OBJECT_CHANGE(cmzn_shaderprogram)(shaderprogram,
				MANAGER_CHANGE_IDENTIFIER(cmzn_shaderprogram));
		}
	}
	else
	{
		if (shaderprogram)
		{
			display_message(ERROR_MESSAGE,
				"cmzn_shaderprogram_set_name.  Invalid shaderprogram name '%s'", name);
		}
		return_code=0;
	}

	return (return_code);
}

