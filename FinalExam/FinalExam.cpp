#include <GL/glut.h>
#include <vector>
#include <cmath>

// 카메라 위치 및 방향
float cameraX = 0.0f;
float cameraY = 1.8f; // 카메라 높이
float cameraZ = 5.0f;
float cameraAngle = 0.0f;
float cameraPitch = 0.0f;

// 이동 속도
float walkSpeed = 0.1f;

// 마우스 상태
bool mouseLeftDown = false;
int mouseLastX, mouseLastY;

// 나무 위치 정보
std::vector<std::pair<float, float>> trees;

// 조명 모드: 0 = 아침, 1 = 저녁
int lightMode = 1;

// 태양 위치
float sunX = 0.0f;
float sunY = 20.0f;
float sunZ = 30.0f;

GLuint signTexture; // 텍스처 저장 변수

// BMP 파일을 텍스처로 로드하는 함수
bool loadBMP(const char* filename) {
    FILE* file;
    if (fopen_s(&file, filename, "rb") != 0) return false;

    unsigned char header[54];
    fread(header, 1, 54, file); // BMP 헤더 읽기

    unsigned int dataPos = *(int*)&header[0x0A]; // 데이터 시작 위치
    unsigned int imageSize = *(int*)&header[0x22]; // 이미지 크기
    unsigned int width = *(int*)&header[0x12];    // 너비
    unsigned int height = *(int*)&header[0x16];   // 높이

    if (imageSize == 0) imageSize = width * height * 3; // 크기 계산
    if (dataPos == 0) dataPos = 54; // 기본 오프셋

    unsigned char* data = new unsigned char[imageSize];
    fread(data, 1, imageSize, file);
    fclose(file);

    // BGR -> RGB 변환
    for (unsigned int i = 0; i < imageSize; i += 3) {
        unsigned char temp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = temp;
    }

    // OpenGL 텍스처 생성
    glGenTextures(1, &signTexture);
    glBindTexture(GL_TEXTURE_2D, signTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data; // 메모리 해제
    return true;
}

// 나무 위치 초기화 함수
void initializeTrees() {
    float spacing = 3.0f; // 나무 간 간격
    float offset = 2.0f; // 중심 경로로부터의 거리

    for (int i = -10; i <= 10; ++i) {
        float zPos = i * spacing;
        trees.push_back({ -offset, zPos }); // 왼쪽 나무
        trees.push_back({ offset, zPos });  // 오른쪽 나무
    }
}

// 나무 그리기 함수
void drawTree(float x, float z) {
    // 나무 줄기
    glColor3f(0.55f, 0.27f, 0.07f);
    glPushMatrix();
    glTranslatef(x, 0.5f, z);
    glScalef(0.2f, 1.0f, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // 나무 잎사귀
    glColor3f(0.18f, 0.80f, 0.44f);
    glPushMatrix();
    glTranslatef(x, 1.5f, z);
    glutSolidSphere(0.6, 16, 16);
    glPopMatrix();
}

// 경로 그리기 함수
void drawPath() {
    glColor3f(0.36f, 0.25f, 0.20f); // 짙은 갈색
    glBegin(GL_QUADS);
    glVertex3f(-1.5f, 0.0f, -50.0f);
    glVertex3f(1.5f, 0.0f, -50.0f);
    glVertex3f(1.5f, 0.0f, 50.0f);
    glVertex3f(-1.5f, 0.0f, 50.0f);
    glEnd();
}

// 표지판 그리기 함수
void drawSign(float x, float z) {
    // 텍스처 활성화
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, signTexture);

    // 표지판 텍스처
    glColor3f(1.0f, 1.0f, 1.0f); // 텍스처 자체 색상 사용
    glPushMatrix();
    glTranslatef(x, 2.0f, z);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.75f, -0.4f, 0.05f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.75f, -0.4f, 0.05f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.75f, 0.4f, 0.05f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.75f, 0.4f, 0.05f);
    glEnd();
    glPopMatrix();

    // 텍스처 비활성화
    glDisable(GL_TEXTURE_2D);

    // 표지판 기둥
    glColor3f(0.3f, 0.3f, 0.3f);
    glPushMatrix();
    glTranslatef(x, 1.0f, z);
    glScalef(0.1f, 2.0f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();
}

void drawSun() {
    glDisable(GL_LIGHTING); // 조명 비활성화
    if (lightMode == 0) { // 낮
        glColor3f(1.0f, 1.0f, 0.0f); // 밝은 노란색
    }
    else if (lightMode == 1) { // 밤
        glColor3f(0.5f, 0.5f, 0.7f); // 어두운 파란색
    }
    glPushMatrix();
    glTranslatef(sunX, sunY, sunZ);
    glutSolidSphere(0.5, 16, 16);
    glPopMatrix();
    glEnable(GL_LIGHTING); // 조명 활성화
}


// 화면 렌더링 함수
void display() {
    if (lightMode == 0) { // 낮
        glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // 하늘색
    }
    else if (lightMode == 1) { // 밤
        glClearColor(0.0f, 0.0f, 0.1f, 1.0f); // 어두운 남색
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // 카메라 설정
    gluLookAt(cameraX, cameraY, cameraZ,
        cameraX + sin(cameraAngle) * cos(cameraPitch),
        cameraY + sin(cameraPitch),
        cameraZ - cos(cameraAngle) * cos(cameraPitch),
        0.0f, 1.0f, 0.0f);

    drawSun();      // 태양 그리기
    drawPath();     // 경로 그리기

    // 나무 그리기
    for (const auto& tree : trees) {
        drawTree(tree.first, tree.second);
    }

    drawSign(0.0f, 0.0f); // 표지판 그리기

    glutSwapBuffers();
}

// 창 크기 변경 처리 함수
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void updateLighting() {
    GLfloat lightAmbient[4];
    GLfloat lightDiffuse[4];
    GLfloat lightSpecular[4];

    if (lightMode == 0) { // 낮
        lightAmbient[0] = 0.6f; lightAmbient[1] = 0.6f; lightAmbient[2] = 0.6f; lightAmbient[3] = 1.0f;
        lightDiffuse[0] = 1.0f; lightDiffuse[1] = 1.0f; lightDiffuse[2] = 0.9f; lightDiffuse[3] = 1.0f;
        lightSpecular[0] = 1.0f; lightSpecular[1] = 1.0f; lightSpecular[2] = 1.0f; lightSpecular[3] = 1.0f;
    }
    else if (lightMode == 1) { // 밤
        lightAmbient[0] = 0.2f; lightAmbient[1] = 0.2f; lightAmbient[2] = 0.3f; lightAmbient[3] = 1.0f;
        lightDiffuse[0] = 0.5f; lightDiffuse[1] = 0.5f; lightDiffuse[2] = 0.6f; lightDiffuse[3] = 1.0f;
        lightSpecular[0] = 0.3f; lightSpecular[1] = 0.3f; lightSpecular[2] = 0.4f; lightSpecular[3] = 1.0f;
    }

    GLfloat lightDirection[] = { sunX, sunY, sunZ, 0.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
}


// 키보드 입력 처리 함수
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': // 앞으로 이동
        cameraX += walkSpeed * sin(cameraAngle);
        cameraZ -= walkSpeed * cos(cameraAngle);
        break;
    case 's': // 뒤로 이동
        cameraX -= walkSpeed * sin(cameraAngle);
        cameraZ += walkSpeed * cos(cameraAngle);
        break;
    case 'a': // 좌회전
        cameraAngle -= 0.05f;
        break;
    case 'd': // 우회전
        cameraAngle += 0.05f;
        break;
    case 'm': // 아침 모드
        lightMode = 0;
        updateLighting();
        break;
    case 'n': // 저녁 모드
        lightMode = 1;
        updateLighting();
        break;
    case 27: // ESC 키 종료
        exit(0);
        break;
    }
    glutPostRedisplay();
}

// 마우스 입력 처리 함수
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseLeftDown = (state == GLUT_DOWN);
        mouseLastX = x;
        mouseLastY = y;
    }
}

// 마우스 이동 처리 함수
void motion(int x, int y) {
    if (mouseLeftDown) {
        float deltaX = (x - mouseLastX) * 0.005f;
        float deltaY = (y - mouseLastY) * 0.005f;

        cameraAngle += deltaX;
        cameraPitch -= deltaY;

        // 수직 시점 제한
        if (cameraPitch > 1.5f) cameraPitch = 1.5f;
        if (cameraPitch < -1.5f) cameraPitch = -1.5f;

        mouseLastX = x;
        mouseLastY = y;

        glutPostRedisplay();
    }
}

// 초기화 함수
void init() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    updateLighting();

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat matShininess[] = { 50.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);

    glClearColor(0.9f, 0.5f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    initializeTrees(); // 나무 위치 초기화

    if (!loadBMP("sign_texture.bmp")) {
        printf("텍스처 로드 실패\n");
    }
    glEnable(GL_TEXTURE_2D);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("OpenGL Park Walk");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}
    

