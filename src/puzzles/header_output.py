"""Keep historical puzzle generators from replacing maintained native data."""
import argparse
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1] / "native"
BANNER = "// Hand-maintained; original generator inputs are not retained.\n"


class HeaderOutput:
    def __init__(self, argv=None):
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--output-dir", type=Path, default=NATIVE,
                            help="Directory for generated headers and include files")
        parser.add_argument("--overwrite-maintained", action="store_true",
                            help="Explicitly permit replacing differing maintained native data")
        args = parser.parse_args(argv)
        self.directory = args.output_dir
        self.overwrite = args.overwrite_maintained

    def write(self, name, content):
        target = self.directory / name
        maintained = NATIVE / name
        if target.exists():
            if target.read_text(encoding="utf8") == content:
                return
            if target.resolve() == maintained.resolve() and not self.overwrite:
                raise SystemExit(
                    f"Refusing to overwrite differing maintained file {target}; "
                    "use --output-dir elsewhere or explicitly pass --overwrite-maintained."
                )
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf8")
