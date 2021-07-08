
class Draw
{
public:
    //r, g, b is the same in all functions. It's Red, Green Blue. The color ranges from 0.0f - 1.0f
    //I can add separate functions if you want me to make a version for color ranges from 0 - 255

    //Draws a string onto the screen. Font is a font (ex: GLUT_BITMAP_HELVETICA_18). Use the end of the funct
    //like printf
    void string(GLfloat x, GLfloat y, GLfloat r, GLfloat g, GLfloat b, void* font, char const* const pFmtText, ...);
    //Draws a filled box. xLen is the length of the left and right sides, yLen is the length of the top and bottom
    void box(GLfloat x, GLfloat y, GLfloat xLen, GLfloat yLen, GLfloat r, GLfloat g, GLfloat b);
    //Same as box, though it draws just the outline. Width is how thick you want the line to be
    void boxLine(GLfloat width, GLfloat x, GLfloat y, GLfloat xLen, GLfloat yLen, GLfloat r, GLfloat g, GLfloat b);
    //Draws a normal line. x1/y1 and x2/y2 are to beginning and ending coords of the line
    void line(GLfloat width, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2, GLfloat r, GLfloat g, GLfloat b);
    //Draws a dot. Size is how large the dot is
    void dot(GLfloat size, GLfloat x, GLfloat y, GLfloat r, GLfloat g, GLfloat b);
};