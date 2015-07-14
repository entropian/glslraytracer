#define GLSL(src) "#version 150 core\n" #src

const char* basicVertSrc = GLSL(
    in vec2 aPosition;
    
    void main()
    {
        gl_Position = vec4(aPosition, 0.0, 1.0);
    }
);

const char* basicFragSrc = GLSL(
    out vec4 outColor;

    float PI = 3.14159265359;
    float MAX_DEPTH = 100000;
    float MIN_T = 0.0001;
    vec3 BACKGROUND_COLOR = vec3(0.1, 0.1, 0.2);

    uniform int MAX_BOUNCE;

    struct ray{
        vec3 origin;
        vec3 direction;
    };

    struct light{
        vec3 position;
        vec3 color;
        float intensity;
    };
    uniform light uLight1;
    uniform light uLight2;
    
    struct material{
        float ka;                // Ambient coefficient
        float kd;                // Diffuse coefficient
        float ks;                // Specular coefficient
        float kt;                // Transmission coefficient
        vec3 color;              
        int matType;             // Material type: 0 = Opaque non-reflective, 1 = reflective, 2 = transmissive
        float ior;               // Index of refraction
    };

    struct plane{
        vec3 point;              // A point that's on the plane
        vec3 normal;             // A unit vector that's perpendicular to the plane
        material mat;
        bool checkered;          // Flag for checker pattern
    };
    uniform plane uPlane1;
    uniform plane uPlane2;
    uniform plane uPlane3;
    uniform plane uPlane4;
    uniform plane uPlane5;
    uniform plane uPlane6;    
    
    struct sphere{
        float radius;
        vec3 center;
        material mat;
    };
    uniform sphere uSphere1;
    uniform sphere uSphere2;
    uniform sphere uSphere3;

    // Struct for ray-object intersection
    struct shadeRec{
        vec3 normal;                
        float t;
        material mat;
    };

    shadeRec planeIntersect(plane p, ray r)
    {
        shadeRec ret;
        float t = dot(p.point - r.origin, p.normal) / dot(r.direction, p.normal);
        if(t > MIN_T)
        {
            ret.t = t;
            ret.normal = p.normal;
            ret.mat = p.mat;
            if(p.checkered)
            {
                vec3 hitPoint = r.origin + r.direction * t;

                int x = int(floor(hitPoint.x/2));
                int z = int(floor(hitPoint.z/2));
                
                if(x % 2 == 0)
                {
                    if(z % 2 == 0)
                        ret.mat.color = vec3(0, 0, 0);
                }else
                {
                    if(z % 2 == 1)
                        ret.mat.color = vec3(0, 0, 0);
                }
            }
        }else
            ret.t = MAX_DEPTH;

        return ret;
    }
    
    shadeRec sphereIntersect(sphere s, ray r)
    {        
        shadeRec ret;

        float t;
        vec3 tmp = r.origin - s.center;
        float a = dot(r.direction, r.direction);
        float b = 2.0 * dot(tmp, r.direction);
        float c = dot(tmp, tmp) - s.radius * s.radius;
        float disc = b * b - 4.0 * a * c;

        if(disc < 0.0)
        {
            ret.t = MAX_DEPTH;
            return ret;
        }else
        {
            float e = sqrt(disc);
            float denom = 2.0 * a;
            t = (-b - e) / denom;

            if(t > MIN_T)
            {
                ret.t = t;
                ret.normal = normalize(tmp + t * r.direction);
                ret.mat = s.mat;
                return ret;
            }

            t = (-b + e) / denom;

            if(t > MIN_T)
            {            
             
                ret.t = t;
                ret.normal = normalize(tmp + t * r.direction);
                ret.mat = s.mat;
                return ret;
            }

        }
        ret.t = MAX_DEPTH;
        return ret;
    }


    shadeRec intersectTest(ray r)
    {
        shadeRec ret;
        ret.t = MAX_DEPTH;
        ret.normal = vec3(0, 0, 0);
        ret.mat.kd = 0;
        ret.mat.ka = 0;
        ret.mat.ks = 0;
        ret.mat.color = BACKGROUND_COLOR;
        
        shadeRec tmp;
        /*
        // front plane
        tmp = planeIntersect(uPlane1, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }
        // left plane
        tmp = planeIntersect(uPlane2, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }
        // right plane
        tmp = planeIntersect(uPlane3, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }
        // back plane
        tmp = planeIntersect(uPlane4, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }
        // top plane
        tmp = planeIntersect(uPlane5, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }
        */
        // bottom plane
        tmp = planeIntersect(uPlane6, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
            //ret.mat.color = vec3(1, 1, 1);
        }        

        // Sphere1
        tmp = sphereIntersect(uSphere1, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }
        
        // Sphere2
        tmp = sphereIntersect(uSphere2, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }

        // Sphere3
        tmp = sphereIntersect(uSphere3, r);
        if(tmp.t < ret.t)
        {
            ret.t = tmp.t;
            ret.normal = tmp.normal;
            ret.mat = tmp.mat;
        }

        return ret;
    }

    bool shadowIntersectTest(ray r, vec3 lightPos)
    {
        float t_max = dot((lightPos - r.origin), r.direction);

        shadeRec tmp;
        /*
        // front plane
        tmp = planeIntersect(uPlane1, r);
        if(tmp.t < t_max)
            return true;

        // left plane
        tmp = planeIntersect(uPlane2, r);
        if(tmp.t < t_max)
            return true;                                    

        // right plane
        tmp = planeIntersect(uPlane3, r);
        if(tmp.t < t_max)
            return true;                        

        // back plane
        tmp = planeIntersect(uPlane4, r);
        if(tmp.t < t_max)
            return true;
        
        // top plane
        tmp = planeIntersect(uPlane5, r);
        if(tmp.t < t_max)
            return true;
        */

        // bottom plane
        tmp = planeIntersect(uPlane6, r);
        if(tmp.t < t_max)
            return true;            

        // Sphere12
        tmp = sphereIntersect(uSphere1, r);
        if(tmp.t < t_max)
            return true;            

        
        // Sphere2
        tmp = sphereIntersect(uSphere2, r);
        if(tmp.t < t_max)
            return true;

        // Sphere3
        tmp = sphereIntersect(uSphere3, r);
        if(tmp.t < t_max)
            return true;        

        return false;        
    }

    // Calculates the direct illumination component of a ray-object intersection
    vec3 directIllum(shadeRec sr, ray r)
    {
        vec3 L = sr.mat.ka * sr.mat.color;

        ray shadowRay;
        vec3 reflectDir;
        vec3 diffContrib;
        vec3 specContrib;
        shadowRay.origin = sr.t * r.direction + r.origin;
        vec3 lightDir = normalize(uLight1.position - shadowRay.origin);
        shadowRay.direction = lightDir;
        diffContrib = sr.mat.kd * sr.mat.color / PI;
        
        if(!shadowIntersectTest(shadowRay, uLight1.position))
        {
            reflectDir = 2*dot(lightDir, sr.normal)*sr.normal - lightDir;        
            specContrib = sr.mat.ks * pow(max(dot(-r.direction, reflectDir), 0), 5) * sr.mat.color;
            L += (diffContrib + specContrib) * (uLight1.color * uLight1.intensity) * dot(sr.normal, lightDir);
        }

        lightDir = normalize(uLight2.position - (sr.t * r.direction + r.origin));
        shadowRay.direction = lightDir;
        if(!shadowIntersectTest(shadowRay, uLight1.position))
        {
            reflectDir = 2*dot(lightDir, sr.normal)*sr.normal - lightDir;
            specContrib = sr.mat.ks * pow(max(dot(-r.direction, reflectDir), 0), 5) * sr.mat.color;
            L += (diffContrib + specContrib) * (uLight1.color * uLight1.intensity) * dot(sr.normal, lightDir);
        }
        return L;
    }

    // Tests for total internal reflection 
    bool tir(shadeRec sr, ray r)
    {
        float cos_thetai = dot(sr.normal, -r.direction);
        float eta = sr.mat.ior;

        if(cos_thetai < 0.0)
            eta = 1.0 / eta;

        return (1.0 - (1.0 - cos_thetai * cos_thetai) / (eta * eta) < 0.0);
    }

    // Returns the direction of a ray that crosses from one medium to another
    vec3 calcRefractedDirection(shadeRec sr, ray r)
    {
        vec3 n = sr.normal;
        float cos_thetai = dot(n, -r.direction);
        float eta = sr.mat.ior;

        if(cos_thetai < 0.0)
        {
            cos_thetai = -cos_thetai;
            n = -n;
            eta = 1.0 / eta;
        }

        float temp = 1.0 - (1.0 - cos_thetai * cos_thetai) / (eta * eta);
        float cos_theta2 = sqrt(temp);
        vec3 wt = r.direction / eta - (cos_theta2 - cos_thetai / eta) * n;
        
        return wt;
    }

    // Calculates the color of a pixel given that the primary ray hits an object in the scene
    vec3 shade(shadeRec sr, ray r)
    {
        vec3 L = directIllum(sr, r);

        for(int i = 0; i < MAX_BOUNCE && sr.mat.matType != 0; i++)
        {
            ray secondary_ray;
            shadeRec secondary_sr;
            secondary_ray.origin = r.origin + r.direction * sr.t;            
            if(sr.mat.matType == 1)
            {
                secondary_ray.direction = 2*dot(-r.direction, sr.normal)*sr.normal + r.direction;
                secondary_sr = intersectTest(secondary_ray);
                vec3 fr = sr.mat.ks * vec3(1, 1, 1);
                if(secondary_sr.t < MAX_DEPTH)
                {
                    //vec3 fr = 0.5f * vec3(1, 1, 1) / dot(secondary_ray.direction, sr.normal);
                    //L += fr * directIllum(secondary_sr, secondary_ray) * dot(secondary_ray.direction, sr.normal);
                    L += fr * directIllum(secondary_sr, secondary_ray);
                }else
                {
                    L += fr * BACKGROUND_COLOR;
                    return L;
                }

            }else
            {
                if(!tir(sr, r))
                {
                    secondary_ray.direction = calcRefractedDirection(sr, r);
                    secondary_sr = intersectTest(secondary_ray);
                    vec3 ft = sr.mat.kt / (sr.mat.ior * sr.mat.ior) * vec3(1, 1, 1);                        
                    if(secondary_sr.t < MAX_DEPTH)
                    {

                        L += ft * directIllum(secondary_sr, secondary_ray);
                    }else
                    {
                        L += ft * BACKGROUND_COLOR;
                        return L;
                    }
                }else
                    return L;

            }
            sr = secondary_sr;
            r = secondary_ray;
        }
        return L;
    }

    void main()
    {
        ray r;
        //float z = sphereIntersect(gl_FragCoord.x / 800.0 / .75 - (0.5 / .75), gl_FragCoord.y / 600.0 - 0.5);
        //r.origin = vec3(gl_FragCoord.x / 1280.0 / 0.75 - (0.5 / .75), gl_FragCoord.y / 960.0 - 0.5, 0.0);

        // Construct ray with the position of the camera and a point on the viewplane
        r.origin = vec3(0, 0, 2);
        r.direction = vec3(gl_FragCoord.x / 1280.0 / 0.75 - (0.5 / .75), gl_FragCoord.y / 960.0 - 0.5, 1.0) - r.origin;

        // Check if the ray hits any of the objects in the scene
        shadeRec sr = intersectTest(r);
        
        if(sr.t < MAX_DEPTH)
        {
            // Ray hits an object
            vec3 color = shade(sr, r);
            outColor = vec4(color, 1.0f);            
        }else
        {
            outColor = vec4(BACKGROUND_COLOR, 1.0f);
        }                
    }
);
