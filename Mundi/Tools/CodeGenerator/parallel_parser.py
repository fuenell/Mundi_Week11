"""
병렬 헤더 파싱 모듈
멀티프로세싱을 사용하여 여러 헤더 파일을 동시에 파싱
"""

from pathlib import Path
from typing import List, Optional, Tuple
from multiprocessing import Pool, cpu_count
import sys
import os

# 모듈 임포트를 위해 현재 디렉토리를 sys.path에 추가
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from header_parser import HeaderParser, ClassInfo

# 전역 HeaderParser 인스턴스 (멀티프로세싱 워커에서 재사용)
_global_parser: Optional[HeaderParser] = None


def _init_worker(project_root: Path):
    """
    멀티프로세싱 워커 초기화 함수
    각 워커 프로세스가 시작될 때 한 번만 호출됨
    """
    global _global_parser
    _global_parser = HeaderParser(project_root)


def parse_single_header(header_path: Path) -> Optional[ClassInfo]:
    """
    단일 헤더 파일 파싱 (멀티프로세싱용 함수)

    전역 _global_parser를 사용하여 파싱 (워커당 한 번만 초기화)

    Args:
        header_path: 파싱할 헤더 파일 경로

    Returns:
        ClassInfo 또는 None
    """
    global _global_parser

    try:
        if _global_parser is None:
            raise RuntimeError("Parser not initialized. Call _init_worker first.")

        class_info = _global_parser.parse_header(header_path)

        if class_info:
            return class_info

        return None

    except Exception as e:
        print(f"[ERROR] Failed to parse {header_path}: {e}")
        return None


class ParallelHeaderParser:
    """병렬 헤더 파일 파서"""

    def __init__(self, project_root: Optional[Path] = None, num_processes: Optional[int] = None):
        """
        Args:
            project_root: 프로젝트 루트 경로
            num_processes: 사용할 프로세스 수 (None이면 CPU 코어 수만큼)
        """
        if project_root is None:
            project_root = Path(__file__).parent.parent.parent

        self.project_root = project_root
        self.num_processes = num_processes or cpu_count()

        print(f"[ParallelParser] Using {self.num_processes} processes")

    def parse_headers(self, header_files: List[Path]) -> List[ClassInfo]:
        """
        여러 헤더 파일을 병렬로 파싱

        Args:
            header_files: 파싱할 헤더 파일 목록

        Returns:
            파싱된 ClassInfo 리스트 (성공한 것만)
        """
        if not header_files:
            return []

        # 파일이 적으면 병렬화 오버헤드가 더 클 수 있음
        if len(header_files) < 4:
            print(f"[ParallelParser] File count ({len(header_files)}) is small, using single process")
            return self._parse_sequential(header_files)

        print(f"[ParallelParser] Parsing {len(header_files)} files with {self.num_processes} processes...")

        # 멀티프로세싱 풀 생성 및 병렬 파싱
        try:
            # initializer로 각 워커 프로세스에서 한 번만 HeaderParser 초기화
            with Pool(processes=self.num_processes, initializer=_init_worker, initargs=(self.project_root,)) as pool:
                results = pool.map(parse_single_header, header_files)

            # None이 아닌 결과만 필터링
            classes = [r for r in results if r is not None]

            print(f"[ParallelParser] Successfully parsed {len(classes)} classes")
            return classes

        except Exception as e:
            print(f"[ParallelParser] Parallel parsing failed: {e}")
            print("[ParallelParser] Falling back to sequential parsing...")
            return self._parse_sequential(header_files)

    def _parse_sequential(self, header_files: List[Path]) -> List[ClassInfo]:
        """순차 파싱 (폴백용)"""
        parser = HeaderParser(self.project_root)
        classes = []

        for header in header_files:
            try:
                class_info = parser.parse_header(header)
                if class_info:
                    classes.append(class_info)
            except Exception as e:
                print(f"[ERROR] Failed to parse {header}: {e}")

        return classes

    def find_reflection_classes(self, source_dir: Path) -> List[ClassInfo]:
        """
        소스 디렉토리에서 모든 헤더 파일을 찾아서 병렬 파싱

        Args:
            source_dir: 소스 디렉토리 경로

        Returns:
            파싱된 ClassInfo 리스트
        """
        print(f"[ParallelParser] Scanning directory: {source_dir}")

        # 모든 헤더 파일 찾기
        header_files = list(source_dir.rglob("*.h"))

        print(f"[ParallelParser] Found {len(header_files)} header files")

        # 병렬 파싱
        return self.parse_headers(header_files)
