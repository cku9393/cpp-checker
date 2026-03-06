# next steps

## 바로 할 일
1. wrapper 5개 채우기
2. `runStaticPipelineCaseDumpAware` 실행
3. 첫 bundle 저장
4. earliest-stage만 디버깅
5. static 통과 후 `runDummyGraftCaseDumpAware` 실행

## 이 bundle이 보장하는 것
- main() 인자 형식이 고정돼 있음
- CMake feature-detection 뼈대가 있음
- failure bundle 읽는 순서가 고정돼 있음
- 실행 프롬프트가 이미 포함돼 있음

## 주의
이 묶음은 로컬 코드베이스에 실제 harness 함수가 이미 있거나, 곧 연결할 것을 전제로 한 실행 뼈대다.
