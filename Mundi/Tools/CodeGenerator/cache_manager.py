"""
코드 생성 캐시 관리 모듈
파일 변경 감지 및 incremental build 지원
"""

import json
import hashlib
from pathlib import Path
from typing import Dict, Set, Optional
from dataclasses import dataclass, asdict


@dataclass
class CacheEntry:
    """캐시 엔트리"""
    file_hash: str          # 파일 내용 해시
    mtime: float            # 수정 시간 (timestamp)
    generated_files: list   # 생성된 파일 목록 [.generated.h, .generated.cpp]


class CacheManager:
    """파일 파싱 결과 캐시 관리 (Incremental Build 지원)"""

    def __init__(self, cache_file: Path):
        """
        Args:
            cache_file: 캐시 파일 경로 (예: output_dir / ".codegen_cache.json")
        """
        self.cache_file = cache_file
        self.cache: Dict[str, CacheEntry] = {}
        self._load_cache()

    def _load_cache(self):
        """캐시 파일 로드"""
        if self.cache_file.exists():
            try:
                data = json.loads(self.cache_file.read_text(encoding='utf-8'))
                # JSON dict -> CacheEntry 변환
                for key, value in data.items():
                    self.cache[key] = CacheEntry(**value)
            except Exception as e:
                print(f"[CacheManager] Failed to load cache: {e}")
                self.cache = {}

    def _save_cache(self):
        """캐시 파일 저장"""
        try:
            # CacheEntry -> dict 변환
            data = {key: asdict(entry) for key, entry in self.cache.items()}
            self.cache_file.parent.mkdir(parents=True, exist_ok=True)
            self.cache_file.write_text(json.dumps(data, indent=2), encoding='utf-8')
        except Exception as e:
            print(f"[CacheManager] Failed to save cache: {e}")

    @staticmethod
    def get_file_hash(file_path: Path) -> str:
        """파일 내용의 MD5 해시 계산"""
        try:
            return hashlib.md5(file_path.read_bytes()).hexdigest()
        except Exception:
            return ""

    def is_file_changed(self, file_path: Path) -> bool:
        """
        파일이 변경되었는지 확인

        Returns:
            True: 파일이 변경됨 (재생성 필요)
            False: 파일이 변경되지 않음 (스킵 가능)
        """
        file_key = str(file_path.resolve())

        # 캐시에 없으면 변경된 것으로 간주
        if file_key not in self.cache:
            return True

        cached_entry = self.cache[file_key]

        # mtime 비교 (빠른 체크)
        try:
            current_mtime = file_path.stat().st_mtime
            if current_mtime != cached_entry.mtime:
                return True
        except Exception:
            return True

        # mtime이 같으면 해시 체크는 생략 (성능 최적화)
        # mtime이 같으면 내용도 같다고 가정
        return False

    def update_cache(self, file_path: Path, generated_files: list):
        """
        캐시 업데이트

        Args:
            file_path: 소스 헤더 파일
            generated_files: 생성된 파일 목록 (Path 객체들)
        """
        file_key = str(file_path.resolve())

        try:
            file_hash = self.get_file_hash(file_path)
            mtime = file_path.stat().st_mtime

            self.cache[file_key] = CacheEntry(
                file_hash=file_hash,
                mtime=mtime,
                generated_files=[str(f) for f in generated_files]
            )

            self._save_cache()
        except Exception as e:
            print(f"[CacheManager] Failed to update cache for {file_path}: {e}")

    def get_changed_files(self, all_files: list) -> Set[Path]:
        """
        변경된 파일 목록 반환

        Args:
            all_files: 검사할 모든 파일 목록 (Path 객체들)

        Returns:
            변경된 파일들의 집합
        """
        changed = set()

        for file_path in all_files:
            if self.is_file_changed(file_path):
                changed.add(file_path)

        return changed

    def clear_cache(self):
        """캐시 초기화"""
        self.cache.clear()
        if self.cache_file.exists():
            self.cache_file.unlink()
        print("[CacheManager] Cache cleared")

    def get_stats(self) -> Dict:
        """캐시 통계 정보"""
        return {
            'total_entries': len(self.cache),
            'cache_file': str(self.cache_file),
            'exists': self.cache_file.exists()
        }
