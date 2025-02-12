from typing import Final
import pathlib

import pytest

import yatest.common as yat

from ydb.library.yql.providers.generic.connector.api.common.data_source_pb2 import EDataSourceKind
from ydb.library.yql.providers.generic.connector.tests.utils.settings import Settings
from ydb.library.yql.providers.generic.connector.tests.utils.dqrun import DqRunner
from ydb.library.yql.providers.generic.connector.tests.utils.kqprun import KqpRunner
from ydb.library.yql.providers.generic.connector.tests.utils.runner import Runner
from ydb.library.yql.providers.generic.connector.tests.utils.clients.postgresql import Client


docker_compose_dir: Final = pathlib.Path("ydb/library/yql/providers/generic/connector/tests/datasource/postgresql")


@pytest.fixture
def settings() -> Settings:
    return Settings.from_env(docker_compose_dir=docker_compose_dir, data_source_kinds=[EDataSourceKind.POSTGRESQL])


@pytest.fixture
def postgresql_client(settings) -> Client:
    return Client(settings.postgresql)


def configure_runner(runner, settings) -> Runner:
    if runner is DqRunner:
        return DqRunner(dqrun_path=yat.build_path("ydb/library/yql/tools/dqrun/dqrun"), settings=settings)
    elif runner is KqpRunner:
        return KqpRunner(kqprun_path=yat.build_path("ydb/tests/tools/kqprun/kqprun"), settings=settings)
    return None
